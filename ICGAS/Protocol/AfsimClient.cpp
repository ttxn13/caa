#include "pch.hpp"
#include "AfsimClient.hpp"

namespace {
bool report_pdu_summary_type(int pdu_type)
{
	return pdu_type == 1 || pdu_type == 2 || pdu_type == 3 || pdu_type == 23;
}
}

QMutex AfsimClient::mutex;
AfsimClient* AfsimClient::instance = NULL;

AfsimClient* AfsimClient::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		if (instance == NULL) {
			instance = new AfsimClient();
		}
		mutex.unlock();
	}
	return instance;
}

void AfsimClient::DstInstance()
{
	delete instance;
	instance = NULL;
}

AfsimClient::AfsimClient()
	: QObject(nullptr)
{
	this->expert = InfoExpert::GetInstance();
	qRegisterMetaType<S_PLAT>("S_PLAT");	// 注册S_PLAT到QT元对象
	qRegisterMetaType<QByteArray>("QByteArray");
	qRegisterMetaType<long long>("long long");
	qRegisterMetaType<qint64>("qint64");
}

AfsimClient::~AfsimClient()
{
	stop_afs_client();
}

bool AfsimClient::start_afs_client()
{
	// 运行中直接返回，避免重复启动线程和socket
	if (is_running) {
		emit sig_afs_client_msg(QString("[%1] [START] [AfSim Client] Started at %2:%3")
			.arg(cur_time_str()).arg(bind_addr).arg(bind_port));
		return true;
	}

	// 校验配置文件IP和port合理性
	bool port_ok = false;
	const QString config_bind_addr = expert->afs_client_addr.trimmed();
	const quint16 config_bind_port = expert->afs_client_port.trimmed().toUShort(&port_ok);
	iter_ms = expert->afs_recv_iter_ms;		// 接收间隔 ms
	if (config_bind_addr.isEmpty() || config_bind_port == 0 || !port_ok) {
		emit sig_afs_client_msg(QString("[%1] [ERROR] [AfSim Client] Invalid AFSIM DIS bind config: afs_addr=%2, afs_port=%3")
			.arg(cur_time_str()).arg(expert->afs_client_addr).arg(expert->afs_client_port));
		return false;
	}

	return start(config_bind_addr, config_bind_port);
}

bool AfsimClient::start(const QString& addr, quint16 port)
{
	if (is_running) {
		emit sig_afs_client_msg(QString("[%1] [START] [AfSim Client] Started at %2:%3")
			.arg(cur_time_str()).arg(bind_addr).arg(bind_port));
		return true;
	}
	if (addr.trimmed().isEmpty() || port == 0) {
		emit sig_afs_client_msg(QString("[%1] [ERROR] [AfSim Client] Invalid AFSIM DIS bind config: afs_addr=%2, afs_port=%3")
			.arg(cur_time_str()).arg(addr).arg(port));
		return false;
	}

	// 启动线程
	if (thread_self != nullptr) {
		if (thread_self->isRunning()) return true;
		delete thread_self;
		thread_self = nullptr;
	}

	bind_addr = addr.trimmed();
	bind_port = port;
	thread_main = QThread::currentThread();
	thread_self = new QThread();

	moveToThread(thread_self);
	connect(thread_self, &QThread::started, this, &AfsimClient::on_start_thread);

	is_running = true;
	thread_self->start();
	return true;
}

void AfsimClient::stop_afs_client()
{
	if (thread_self == nullptr) {
		is_running = false;
		return;
	}

	if (thread_self->isRunning()) {
		if (QThread::currentThread() == thread_self) {
			on_stop_thread();
			return;
		}

		if (thread() == thread_self) {
			QMetaObject::invokeMethod(this, "on_stop_thread", Qt::BlockingQueuedConnection);
		}
		else {
			thread_self->quit();
		}
		thread_self->wait(3000);
	}

	delete thread_self;
	thread_self = nullptr;
	is_running = false;
}

void AfsimClient::on_start_thread()
{
	// socket和timer必须在AfsimClient子线程中创建
	const QHostAddress bind_address(bind_addr);
	socket_udp = new QUdpSocket(this);
	if (!socket_udp->bind(bind_address, bind_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		emit sig_afs_client_msg(QString("[%1] [ERROR] [AfSim Client] Bind failed at %2:%3, %4")
			.arg(cur_time_str()).arg(bind_addr).arg(bind_port).arg(socket_udp->errorString()));
		delete socket_udp;
		socket_udp = nullptr;
		is_running = false;
		moveToThread(thread_main);
		thread_self->quit();
		return;
	}
	connect(socket_udp, &QUdpSocket::readyRead, this, &AfsimClient::on_data_recv);

	// 发送状态
	recv_pdu_count.clear();
	recv_pdu_summary_count.clear();
	last_summary_time_ms = QDateTime::currentMSecsSinceEpoch();
	emit sig_afs_client_msg(QString("[%1] [START] [AfSim Client] Started at %2:%3")
		.arg(cur_time_str()).arg(bind_addr).arg(bind_port));
}

void AfsimClient::on_stop_thread()
{
	if (socket_udp != nullptr) {
		socket_udp->close();
		delete socket_udp;
		socket_udp = nullptr;
	}

	if (is_running) {
		flush_recv_summary(QDateTime::currentMSecsSinceEpoch(), true);
		is_running = false;
		recv_pdu_count.clear();
		recv_pdu_summary_count.clear();
		emit sig_afs_client_msg(QString("[%1] [STOP] [AfSim Client] Stoped.").arg(cur_time_str()));
	}

	moveToThread(thread_main);
	thread_self->quit();
}

void AfsimClient::on_data_recv()
{
	if (socket_udp == nullptr)	return;

	// 一次触发读完所有积压包
	while (socket_udp->hasPendingDatagrams()) {
		QByteArray data;
		data.resize(static_cast<int>(socket_udp->pendingDatagramSize()));

		QHostAddress peer_address;
		quint16 peer_port = 0;
		const qint64 read_size = socket_udp->readDatagram(data.data(), data.size(), &peer_address, &peer_port);
		const long long recv_time_ms = QDateTime::currentMSecsSinceEpoch();
		if (read_size < 0) {
			emit sig_afs_client_msg(QString("[%1] [ERROR] [AFS Client] Read UDP data failed: %2")
				.arg(cur_time_str()).arg(socket_udp->errorString()));
			continue;
		}
		if (data.size() < PDU_HEADER_LEN) {
			emit sig_afs_client_msg(QString("[%1] [ERROR] [AFS Client] Parse failed: DIS header too short: %2 bytes\nFrom: %3:%4\nHEX Preview: %5")
				.arg(cur_time_str()).arg(data.size()).arg(peer_address.toString()).arg(peer_port).arg(to_hex_preview(data)));
			continue;
		}

		// 数据分发处理
		data.truncate(static_cast<int>(read_size));
		emit sig_datagram_received(data, peer_address.toString(), peer_port, recv_time_ms);
		deal_pdu_div(data, recv_time_ms);
	}

	flush_recv_summary(QDateTime::currentMSecsSinceEpoch());
}

void AfsimClient::deal_pdu_div(const QByteArray& data, const long long recv_time_ms)
{
	S_PDU_HEADER pdu_header;
	UtilDisPdu::recv_pdu_header(data, pdu_header);

	const int pdu_type = pdu_header.pdu_type;
	const int pdu_count = ++recv_pdu_count[pdu_type];
	++recv_pdu_summary_count[pdu_type];

	// 共0 + 1-72 = 73种PDU类型，暂未完整实现，其中0类型无实际含义
	switch (pdu_header.pdu_type) {
	case 1: {
		S_PLAT plat;
		if (!UtilDisPdu::deal_pdu_entity_state(data, plat)) {
			emit sig_afs_client_msg(QString("[%1] [ERROR] [AFS Client] DIS PDU-%2 parse failed. Num %3.")
				.arg(cur_time_str()).arg(pdu_type, 2, 10, QChar('0')).arg(pdu_count));
			return;
		}

		if (!plat.list_motion.empty()) {
			plat.list_motion.back().update_time_ms = recv_time_ms;
		}
		emit sig_update_entity_state(plat);
		break;
	}
	default:
		break;
	}
}

void AfsimClient::flush_recv_summary(const long long cur_time_ms, const bool force)
{
	if (recv_pdu_summary_count.empty()) return;
	if (!force && cur_time_ms - last_summary_time_ms < 1000) return;

	for (const auto& item : recv_pdu_summary_count) {
		const int pdu_type = item.first;
		if (!report_pdu_summary_type(pdu_type)) continue;
		const int pdu_count = recv_pdu_count[pdu_type];
		emit sig_afs_client_msg(QString("[%1] [RECV] [AFS Client] DIS PDU-%2 recv. Num %3.")
			.arg(cur_time_str()).arg(pdu_type, 2, 10, QChar('0')).arg(pdu_count));
	}

	recv_pdu_summary_count.clear();
	last_summary_time_ms = cur_time_ms;
}

// ---------- 工具函数 ----------
QString AfsimClient::cur_time_str() const
{
	//return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
	return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}

QString AfsimClient::to_hex_preview(const QByteArray& datagram) const
{
	QString text = QString::fromLatin1(datagram.toHex(' ').toUpper());
	return text;
}
