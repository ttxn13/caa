#include "pch.hpp"
#include "TacviewServer.hpp"

namespace {
const S_MOTION_FRAME* latest_motion(const S_PLAT& plat)
{
	return plat.list_motion.empty() ? nullptr : &plat.list_motion.back();
}

const S_MOTION_FRAME* latest_motion(const S_WEAPON& weapon)
{
	return weapon.list_motion.empty() ? nullptr : &weapon.list_motion.back();
}
}

QMutex TacviewServer::mutex;
TacviewServer* TacviewServer::instance = NULL;

TacviewServer* TacviewServer::GetInstance()
{
	if (instance == NULL) {
		mutex.lock();
		if (instance == NULL) {
			instance = new TacviewServer();
		}
		mutex.unlock();
	}
	return instance;
}

void TacviewServer::DstInstance()
{
	delete instance;
	instance = NULL;
}

TacviewServer::TacviewServer()
	: QObject(nullptr)
{
	this->expert		= InfoExpert::GetInstance();
	this->module_db		= ModuleDatabase::GetInstance();
}

TacviewServer::~TacviewServer()
{
	stop_tac_server();
}

bool TacviewServer::start_tac_server()
{
	// 运行中直接返回，避免重复启动线程和端口监听
	if (is_running) {
		emit sig_tac_server_msg(QString("[%1] [START] [Tacview Server] Started at %2:%3")
			.arg(cur_time_str()).arg(bind_addr).arg(bind_port));
		return true;
	}

	if (!init_config()) return false;

	if (thread_self != nullptr) {
		if (thread_self->isRunning()) return true;
		delete thread_self;
		thread_self = nullptr;
	}

	thread_main = QThread::currentThread();
	thread_self = new QThread();

	moveToThread(thread_self);
	connect(thread_self, &QThread::started, this, &TacviewServer::on_start_thread);

	is_running = true;
	thread_self->start();
	return true;
}

void TacviewServer::stop_tac_server()
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

void TacviewServer::on_start_thread()
{
	// socket和timer必须在TacviewServer子线程中创建
	server_tcp = new QTcpServer(this);
	if (!server_tcp->listen(host_addr(bind_addr), bind_port)) {
		emit sig_tac_server_msg(QString("[%1] [ERROR] [Tacview Server] Listen failed at %2:%3, %4")
			.arg(cur_time_str()).arg(bind_addr).arg(bind_port).arg(server_tcp->errorString()));
		delete server_tcp;
		server_tcp = nullptr;
		is_running = false;
		moveToThread(thread_main);
		thread_self->quit();
		return;
	}
	connect(server_tcp, &QTcpServer::newConnection, this, &TacviewServer::on_new_connection);

	timer_send = new QTimer(this);
	timer_send->setInterval(iter_ms);
	connect(timer_send, &QTimer::timeout, this, &TacviewServer::on_timer_send);
	timer_send->start();

	emit sig_tac_server_msg(QString("[%1] [START] [Tacview Server] Started at %2:%3")
		.arg(cur_time_str()).arg(bind_addr).arg(bind_port));
}

void TacviewServer::on_stop_thread()
{
	clear_timer();
	clear_client();
	clear_server();

	frame_id = 0;
	is_connected = false;
	if (is_running) {
		is_running = false;
		emit sig_tac_server_msg(QString("[%1] [STOP] [Tacview Server] Stoped.").arg(cur_time_str()));
	}

	moveToThread(thread_main);
	thread_self->quit();
}

void TacviewServer::on_new_connection()
{
	if (server_tcp == nullptr) return;

	// 只保留一个Tacview连接，新的连接会替换旧连接
	clear_client();

	client_tcp = server_tcp->nextPendingConnection();
	if (client_tcp == nullptr) return;

	connect(client_tcp, &QTcpSocket::readyRead, this, &TacviewServer::on_client_ready_read);
	connect(client_tcp, &QTcpSocket::disconnected, this, &TacviewServer::on_client_disconnected);

	send_handshake();

	emit sig_tac_server_msg(QString("[%1] [STATE] [TAC Server] Connected from %2:%3")
		.arg(cur_time_str()).arg(client_tcp->peerAddress().toString()).arg(client_tcp->peerPort()));
}

void TacviewServer::on_client_ready_read()
{
	if (client_tcp == nullptr) return;
	client_tcp->readAll();

	// Tacview回包后发送ACMI头，之后由timer定时发送态势帧
	if (is_connected) return;
	start_time = QDateTime::currentDateTimeUtc();
	frame_id = 0;
	is_connected = true;
	send_acmi_header();

	emit sig_tac_server_msg(QString("[%1] [STATE] [TAC Server] ACMI header sent").arg(cur_time_str()));
}

void TacviewServer::on_client_disconnected()
{
	is_connected = false;
	active_tac_object_ids.clear();
	frame_id = 0;
	if (client_tcp != nullptr) {
		client_tcp->deleteLater();
		client_tcp = nullptr;
	}
	emit sig_tac_server_msg(QString("[%1] [STATE] [TAC Server] Disconnected").arg(cur_time_str()));
}

void TacviewServer::on_timer_send()
{
	if (!is_connected || client_tcp == nullptr) return;
	if (client_tcp->state() != QAbstractSocket::ConnectedState) return;

	if (module_db == nullptr) return;
	const std::map<QString, S_PLAT>& plat_list = module_db->list_plat_raw;
	const std::map<QString, S_WEAPON>& weapon_list = module_db->list_weapon_raw;

	QByteArray frame;
	const double record_time_s = static_cast<double>(frame_id * iter_ms) / 1000.0;
	frame.append(QString("#%1\n").arg(record_time_s, 0, 'f', 3).toUtf8());

	std::set<QString> current_object_ids;
	for (const auto& item : plat_list) {
		const S_PLAT& plat = item.second;
		if (!active_tac_object(plat)) continue;
		current_object_ids.insert(object_id(plat));
		append_object_frame(frame, item.second);
	}
	for (const auto& item : weapon_list) {
		const S_WEAPON& weapon = item.second;
		if (!active_tac_object(weapon)) continue;
		current_object_ids.insert(object_id(weapon));
		append_object_frame(frame, weapon);
	}
	for (const QString& old_id : active_tac_object_ids) {
		if (current_object_ids.find(old_id) == current_object_ids.end()) {
			append_remove_object_frame(frame, old_id);
		}
	}
	if (current_object_ids.empty() && active_tac_object_ids.empty()) return;

	const qint64 write_size = client_tcp->write(frame, frame.size());
	if (write_size != frame.size()) {
		emit sig_tac_server_msg(QString("[%1] [ERROR] [TAC Server] Tacview boardcast failed: %2")
			.arg(cur_time_str()).arg(client_tcp->errorString()));
		return;
	}
	client_tcp->flush();
	active_tac_object_ids = current_object_ids;
	frame_id++;
	emit sig_tac_server_msg(QString("[%1] [SHOW] [TAC Server] Tacview boardcast. Num %2")
		.arg(cur_time_str()).arg(frame_id));
}

bool TacviewServer::init_config()
{
	bool port_ok = false;
	bind_addr = expert->tac_server_addr.trimmed();
	bind_port = expert->tac_server_port.trimmed().toUShort(&port_ok);
	bind_user = expert->tac_server_user.trimmed();
	iter_ms = std::max(1, expert->tac_send_iter_ms);
	if (bind_user.isEmpty()) bind_user = "ICGAS";

	if (bind_addr.isEmpty() || bind_port == 0 || !port_ok) {
		emit sig_tac_server_msg(QString("[%1] [ERROR] [Tacview Server] Invalid Tacview server config: tac_addr=%2, tac_port=%3")
			.arg(cur_time_str()).arg(expert->tac_server_addr).arg(expert->tac_server_port));
		return false;
	}
	return true;
}

void TacviewServer::clear_client()
{
	if (client_tcp == nullptr) return;

	disconnect(client_tcp, nullptr, this, nullptr);
	client_tcp->disconnectFromHost();
	client_tcp->close();
	delete client_tcp;
	client_tcp = nullptr;
	is_connected = false;
}

void TacviewServer::clear_server()
{
	if (server_tcp == nullptr) return;

	server_tcp->close();
	delete server_tcp;
	server_tcp = nullptr;
}

void TacviewServer::clear_timer()
{
	if (timer_send == nullptr) return;

	timer_send->stop();
	delete timer_send;
	timer_send = nullptr;
}

void TacviewServer::send_handshake()
{
	if (client_tcp == nullptr) return;

	QByteArray data = QString("XtraLib.Stream.0\nTacview.RealTimeTelemetry.0\n%1\n")
		.arg(bind_user).toUtf8();
	data.append('\0');
	client_tcp->write(data, data.size());
	client_tcp->flush();
}

void TacviewServer::send_acmi_header()
{
	if (client_tcp == nullptr) return;

	QByteArray data;
	data.append("FileType=text/acmi/tacview\n");
	data.append("FileVersion=2.2\n");
	data.append(QString("0,ReferenceTime=%1\n")
		.arg(start_time.toString("yyyy-MM-ddTHH:mm:ssZ")).toUtf8());
	client_tcp->write(data, data.size());
	client_tcp->flush();
}

QString TacviewServer::cur_time_str() const
{
	return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}

QString TacviewServer::tac_color(const E_SIDE side) const
{
	switch (side) {
	default					:return "Orange"	; break;
	case E_SIDE::RED	:return "Red"		; break;
	case E_SIDE::BLUE	:return "Blue"		; break;
	case E_SIDE::WHITE	:return "White"		; break;
	}
}

QString TacviewServer::tac_type(const E_PLAT_TYPE type) const
{
	switch (type) {
	default					:return "Misc+Other"		; break;
	case E_PLAT_TYPE::MIS	:return "Weapon+Missile"	; break;
	case E_PLAT_TYPE::CAR	:return "Sea+Watercraft"	; break;
	case E_PLAT_TYPE::DST	:return "Sea+Watercraft"	; break;
	case E_PLAT_TYPE::FRI	:return "Sea+Watercraft"	; break;
	case E_PLAT_TYPE::WAN	:return "Air+FixedWing"	; break;
	case E_PLAT_TYPE::ELC	:return "Air+FixedWing"	; break;
	case E_PLAT_TYPE::FLT	:return "Air+FixedWing"	; break;
	case E_PLAT_TYPE::BOM	:return "Air+FixedWing"	; break;
	}
}

QString TacviewServer::tac_platform_type_code(const E_PLAT_TYPE type) const
{
	return UtilEnum::trans_type_en(type);
}

QString TacviewServer::tac_weapon_type(const E_WEAPON_TYPE type) const
{
	Q_UNUSED(type);
	return QStringLiteral("Weapon+Missile");
}

QString TacviewServer::tac_weapon_type_code(const E_WEAPON_TYPE type) const
{
	return UtilEnum::trans_weapon_type_en(type);
}

QString TacviewServer::tac_model_name(const S_PLAT& plat) const
{
	const QString type_name = plat.type_name.trimmed();
	if (!type_name.isEmpty()) {
		return type_name;
	}

	switch (plat.type) {
	default					:return QStringLiteral("Other");
	case E_PLAT_TYPE::MIS	:return QStringLiteral("Missile");
	case E_PLAT_TYPE::CAR	:return QStringLiteral("Aircraft Carrier");
	case E_PLAT_TYPE::DST	:return QStringLiteral("Destroyer");
	case E_PLAT_TYPE::FRI	:return QStringLiteral("Frigate");
	case E_PLAT_TYPE::WAN	:return QStringLiteral("E-3");
	case E_PLAT_TYPE::ELC	:return QStringLiteral("F-18");
	case E_PLAT_TYPE::FLT	:return QStringLiteral("F-15");
	case E_PLAT_TYPE::BOM	:return QStringLiteral("B-52");
	}
}

QString TacviewServer::tac_model_name(const S_WEAPON& weapon) const
{
	const QString type_name = weapon.type_name.trimmed();
	if (!type_name.isEmpty()) {
		return type_name;
	}

	switch (weapon.type) {
	default					:return QStringLiteral("Missile");
	case E_WEAPON_TYPE::AAM	:return QStringLiteral("AAM");
	case E_WEAPON_TYPE::SAM	:return QStringLiteral("SAM");
	case E_WEAPON_TYPE::ASM	:return QStringLiteral("ASM");
	}
}

QString TacviewServer::tac_shape(const S_PLAT& plat) const
{
	const QString model_name = tac_model_name(plat);
	if (model_name.compare(QStringLiteral("CVN-59"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("Watercraft.CVN-59.obj");
	}
	if (model_name.compare(QStringLiteral("052D"), Qt::CaseInsensitive) == 0 ||
		model_name.compare(QStringLiteral("Type 052D"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("Watercraft.Type 052D.obj");
	}
	if (model_name.compare(QStringLiteral("E-3"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("FixedWing.E-3.obj");
	}
	if (model_name.compare(QStringLiteral("F-18E"), Qt::CaseInsensitive) == 0 ||
		model_name.compare(QStringLiteral("F/A-18E"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("FixedWing.F-18E.obj");
	}
	if (model_name.compare(QStringLiteral("F-15"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("FixedWing.F-15.obj");
	}
	if (model_name.compare(QStringLiteral("B-52"), Qt::CaseInsensitive) == 0) {
		return QStringLiteral("FixedWing.B-52.obj");
	}

	switch (plat.type) {
	default					:return QString();
	case E_PLAT_TYPE::CAR	:return QStringLiteral("Watercraft.CVN-59.obj");
	case E_PLAT_TYPE::DST	:return QStringLiteral("Watercraft.Type 052D.obj");
	case E_PLAT_TYPE::WAN	:return QStringLiteral("FixedWing.E-3.obj");
	case E_PLAT_TYPE::ELC	:return QStringLiteral("FixedWing.F-18E.obj");
	case E_PLAT_TYPE::FLT	:return QStringLiteral("FixedWing.F-15.obj");
	case E_PLAT_TYPE::BOM	:return QStringLiteral("FixedWing.B-52.obj");
	}
}

QString TacviewServer::tac_text(const QString& value) const
{
	QString text = value.trimmed();
	text.replace('\\', '/');
	text.replace(',', ' ');
	text.replace('|', ' ');
	text.replace('\r', ' ');
	text.replace('\n', ' ');
	return text;
}

double TacviewServer::tac_pitch_deg(const S_PLAT& plat) const
{
	Q_UNUSED(plat);
	return 0.0;
}

bool TacviewServer::active_tac_object(const S_PLAT& plat) const
{
	return plat.valid && !plat.plt_id.trimmed().isEmpty() && latest_motion(plat) != nullptr;
}

bool TacviewServer::active_tac_object(const S_WEAPON& weapon) const
{
	return weapon.valid && !weapon.weapon_id.trimmed().isEmpty() && latest_motion(weapon) != nullptr;
}

QString TacviewServer::object_name(const S_PLAT& plat) const
{
	return tac_text(tac_model_name(plat));
}

QString TacviewServer::object_id(const S_PLAT& plat) const
{
	const QString raw_id = plat.plt_id.trimmed();
	if (raw_id.isEmpty()) return "0";

	bool numeric_id = false;
	const quint64 numeric_value = raw_id.toULongLong(&numeric_id);
	if (numeric_id && numeric_value > 0) return raw_id;

	return hashed_object_id(raw_id, plat.type == E_PLAT_TYPE::MIS ? 2000000000u : 1000000000u);
}

QString TacviewServer::object_id(const S_WEAPON& weapon) const
{
	const QString raw_id = weapon.weapon_id.trimmed();
	if (raw_id.isEmpty()) return "0";

	bool numeric_id = false;
	const quint64 numeric_value = raw_id.toULongLong(&numeric_id);
	if (numeric_id && numeric_value > 0) return raw_id;

	return hashed_object_id(raw_id, 2000000000u);
}

QString TacviewServer::hashed_object_id(const QString& raw_id, quint32 type_prefix) const
{
	quint32 hash = 2166136261u;
	const QByteArray id_bytes = raw_id.toUtf8();
	for (const char ch : id_bytes) {
		hash ^= static_cast<quint8>(ch);
		hash *= 16777619u;
	}

	return QString::number(type_prefix + (hash % 900000000u));
}

QString TacviewServer::object_label(const S_PLAT& plat) const
{
	const QString model_name = tac_model_name(plat);
	const QString raw_id = plat.plt_id.trimmed();
	if (raw_id.isEmpty()) return tac_text(model_name);
	if (model_name.isEmpty() || raw_id.compare(model_name, Qt::CaseInsensitive) == 0) {
		return tac_text(raw_id);
	}
	return tac_text(QStringLiteral("%1 %2").arg(model_name, raw_id));
}

QString TacviewServer::object_label(const S_WEAPON& weapon) const
{
	const QString model_name = tac_model_name(weapon);
	const QString raw_id = weapon.weapon_id.trimmed();
	if (raw_id.isEmpty()) return tac_text(model_name);
	if (model_name.isEmpty() || raw_id.compare(model_name, Qt::CaseInsensitive) == 0) {
		return tac_text(raw_id);
	}
	return tac_text(QStringLiteral("%1 %2").arg(model_name, raw_id));
}

void TacviewServer::append_object_frame(QByteArray& frame, const S_PLAT& plat) const
{
	if (!active_tac_object(plat)) return;
	const S_MOTION_FRAME* motion = latest_motion(plat);
	if (motion == nullptr) return;

	const double lon_deg = motion->pos_lla.lon_rad RAD2DEG;
	const double lat_deg = motion->pos_lla.lat_rad RAD2DEG;
	const double alt_m = motion->pos_lla.alt_m;
	const double roll_deg = 0.0;
	const double pitch_deg = tac_pitch_deg(plat);
	const double yaw_deg = UtilCoor::norm_rad_0_2pi(motion->vel_loc.track_ang_rad) RAD2DEG;
	const QString label = object_label(plat);
	const QString type_code = tac_text(tac_platform_type_code(plat.type));
	const QString model_name = tac_text(tac_model_name(plat));
	const QString shape = tac_shape(plat);
	const bool hide_ship_engagement_range =
		plat.type == E_PLAT_TYPE::CAR ||
		plat.type == E_PLAT_TYPE::DST ||
		plat.type == E_PLAT_TYPE::FRI;

	QString object_line = QString("%1,T=%2|%3|%4|%5|%6|%7,Type=%8,Name=%9,ShortName=%9,Pilot=%10,Label=%11")
		.arg(object_id(plat)).arg(lon_deg, 0, 'f', 9).arg(lat_deg, 0, 'f', 9).arg(alt_m, 0, 'f', 2)
		.arg(roll_deg, 0, 'f', 2).arg(pitch_deg, 0, 'f', 2).arg(yaw_deg, 0, 'f', 2).arg(tac_type(plat.type))
		.arg(label, type_code, model_name);
	if (!shape.isEmpty()) {
		object_line += QStringLiteral(",Shape=") + shape;
	}
	if (hide_ship_engagement_range) {
		object_line += QStringLiteral(",EngagementMode=0,EngagementMode2=0,EngagementRange=0,EngagementRange2=0");
	}
	object_line += QStringLiteral(",Color=") + tac_color(plat.side) + QStringLiteral("\n");
	frame.append(object_line.toUtf8());
}

void TacviewServer::append_object_frame(QByteArray& frame, const S_WEAPON& weapon) const
{
	if (!active_tac_object(weapon)) return;
	const S_MOTION_FRAME* motion = latest_motion(weapon);
	if (motion == nullptr) return;

	const double lon_deg = motion->pos_lla.lon_rad RAD2DEG;
	const double lat_deg = motion->pos_lla.lat_rad RAD2DEG;
	const double alt_m = motion->pos_lla.alt_m;
	const double roll_deg = 0.0;
	const double pitch_deg = motion->vel_loc.path_ang_rad RAD2DEG;
	const double yaw_deg = UtilCoor::norm_rad_0_2pi(motion->vel_loc.track_ang_rad) RAD2DEG;
	const QString label = object_label(weapon);
	const QString type_code = tac_text(tac_weapon_type_code(weapon.type));
	const QString model_name = tac_text(tac_model_name(weapon));

	const QString object_line = QString("%1,T=%2|%3|%4|%5|%6|%7,Type=%8,Name=%9,ShortName=%9,Pilot=%10,Label=%11,Color=%12\n")
		.arg(object_id(weapon)).arg(lon_deg, 0, 'f', 9).arg(lat_deg, 0, 'f', 9).arg(alt_m, 0, 'f', 2)
		.arg(roll_deg, 0, 'f', 2).arg(pitch_deg, 0, 'f', 2).arg(yaw_deg, 0, 'f', 2).arg(tac_weapon_type(weapon.type))
		.arg(label, type_code, model_name, tac_color(weapon.side));
	frame.append(object_line.toUtf8());
}

void TacviewServer::append_remove_object_frame(QByteArray& frame, const QString& id) const
{
	const QString object_id = id.trimmed();
	if (object_id.isEmpty()) return;
	frame.append(QStringLiteral("-%1\n").arg(object_id).toUtf8());
}

QHostAddress TacviewServer::host_addr(const QString& addr) const
{
	const QString value = addr.trimmed();
	if (value.compare("localhost", Qt::CaseInsensitive) == 0) return QHostAddress(QHostAddress::LocalHost);
	if (value.compare("127.0.0.1", Qt::CaseInsensitive) == 0) return QHostAddress(QHostAddress::LocalHost);

	QHostAddress host(value);
	if (host.isNull()) return QHostAddress(QHostAddress::AnyIPv4);
	return host;
}
