#include "pch.hpp"
#include "AfsimServer.hpp"

QMutex AfsimServer::mutex;
AfsimServer* AfsimServer::instance = NULL;

AfsimServer* AfsimServer::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		if (instance == NULL) {
			instance = new AfsimServer();
		}
		mutex.unlock();
	}
	return instance;
}

void AfsimServer::DstInstance()
{
	delete instance;
	instance = NULL;
}

AfsimServer::AfsimServer()
	: QObject(nullptr)
{
	this->expert = InfoExpert::GetInstance();
	qRegisterMetaType<QByteArray>("QByteArray");
	qRegisterMetaType<qint64>("qint64");
}

AfsimServer::~AfsimServer()
{
	stop_afs_server();
}

bool AfsimServer::start_afs_server()
{
	if (is_running) {
		emit sig_afs_server_msg(QString("[%1] [START] [AfSim Server] Started at %2:%3")
			.arg(cur_time_str()).arg(target_addr).arg(target_port));
		return true;
	}

	bool port_ok = false;
	const QString config_target_addr = expert->afs_server_addr.trimmed();
	const quint16 config_target_port = expert->afs_server_port.trimmed().toUShort(&port_ok);
	iter_ms = expert->afs_send_iter_ms;
	if (config_target_addr.isEmpty() || config_target_port == 0 || !port_ok) {
		emit sig_afs_server_msg(QString("[%1] [ERROR] [AfSim Server] Invalid AFSIM control config: afs_addr=%2, afs_port=%3")
			.arg(cur_time_str()).arg(expert->afs_server_addr).arg(expert->afs_server_port));
		return false;
	}

	return start(config_target_addr, config_target_port);
}

bool AfsimServer::start(const QString& addr, quint16 port)
{
	if (is_running) {
		emit sig_afs_server_msg(QString("[%1] [START] [AfSim Server] Started at %2:%3")
			.arg(cur_time_str()).arg(target_addr).arg(target_port));
		return true;
	}
	if (addr.trimmed().isEmpty() || port == 0) {
		emit sig_afs_server_msg(QString("[%1] [ERROR] [AfSim Server] Invalid AFSIM control config: afs_addr=%2, afs_port=%3")
			.arg(cur_time_str()).arg(addr).arg(port));
		return false;
	}

	if (thread_self != nullptr) {
		if (thread_self->isRunning()) return true;
		delete thread_self;
		thread_self = nullptr;
	}

	target_addr = addr.trimmed();
	target_port = port;
	thread_main = QThread::currentThread();
	thread_self = new QThread();

	moveToThread(thread_self);
	connect(thread_self, &QThread::started, this, &AfsimServer::on_start_thread);

	is_running = true;
	thread_self->start();
	return true;
}

void AfsimServer::stop_afs_server()
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

bool AfsimServer::send_datagram(const QByteArray& data)
{
	if (!is_running || thread_self == nullptr || !thread_self->isRunning()) {
		emit sig_afs_server_msg(QString("[%1] [ERROR] [AfSim Server] Send failed: UDP output not started.")
			.arg(cur_time_str()));
		return false;
	}

	QMetaObject::invokeMethod(this, "on_send_datagram", Qt::QueuedConnection, Q_ARG(QByteArray, data));
	return true;
}

void AfsimServer::on_start_thread()
{
	socket_udp = new QUdpSocket(this);
	sent_count = 0;
	emit sig_afs_server_msg(QString("[%1] [START] [AfSim Server] Started at %2:%3")
		.arg(cur_time_str()).arg(target_addr).arg(target_port));
}

void AfsimServer::on_stop_thread()
{
	if (socket_udp != nullptr) {
		socket_udp->close();
		delete socket_udp;
		socket_udp = nullptr;
	}

	if (is_running) {
		is_running = false;
		emit sig_afs_server_msg(QString("[%1] [STOP] [AfSim Server] Stoped.").arg(cur_time_str()));
	}

	moveToThread(thread_main);
	thread_self->quit();
}

void AfsimServer::on_send_datagram(const QByteArray& data)
{
	if (socket_udp == nullptr) {
		emit sig_afs_server_msg(QString("[%1] [ERROR] [AfSim Server] Send failed: socket is null.")
			.arg(cur_time_str()));
		return;
	}

	const QHostAddress address(target_addr);
	const qint64 written = socket_udp->writeDatagram(data, address, target_port);
	const qint64 send_time_ms = QDateTime::currentMSecsSinceEpoch();
	if (written < 0) {
		emit sig_afs_server_msg(QString("[%1] [ERROR] [AfSim Server] Send failed: %2")
			.arg(cur_time_str()).arg(socket_udp->errorString()));
		return;
	}

	++sent_count;
	emit sig_datagram_sent(data, target_addr, target_port, send_time_ms);
	if (data.size() >= DIS_PDU_HEADER_LEN) {
		const int pdu_type = UtilDisPdu::read_u8(data, 2);
		emit sig_afs_server_msg(QString("[%1] [SEND] [AfSim Server] DIS PDU-%2 send. Num %3.")
			.arg(cur_time_str()).arg(pdu_type, 2, 10, QChar('0')).arg(sent_count));
	}
	else {
		emit sig_afs_server_msg(QString("[%1] [SEND] [AfSim Server] UDP datagram send. Num %2.")
			.arg(cur_time_str()).arg(sent_count));
	}
}

QString AfsimServer::cur_time_str() const
{
	return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}
