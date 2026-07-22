#pragma once
#include "pch.hpp"
#include "../Core/IncCore.hpp"

class AfsimServer : public QObject
{
	Q_OBJECT

public:
	static AfsimServer*	GetInstance();
	static void			DstInstance();
	AfsimServer();
	~AfsimServer();

	bool start_afs_server();
	void stop_afs_server();
	bool start(const QString& target_addr, quint16 target_port);
	void stop() { stop_afs_server(); }
	bool running() const { return is_running; }
	bool send_datagram(const QByteArray& data);

signals:
	void sig_afs_server_msg(const QString& info);
	void sig_datagram_sent(const QByteArray& data, const QString& target, quint16 port, qint64 time_ms);

private slots:
	void on_start_thread();
	void on_stop_thread();
	void on_send_datagram(const QByteArray& data);

private:
	QString cur_time_str() const;

private:
	static QMutex mutex;
	static AfsimServer* instance;

	QThread* thread_main	= nullptr	;
	QThread* thread_self	= nullptr	;

	InfoExpert* expert		= nullptr	;
	QUdpSocket* socket_udp	= nullptr	;

	QString target_addr	= ""	;
	quint16 target_port	= 0		;
	int iter_ms			= 100	;
	bool is_running		= false	;
	int sent_count		= 0		;
};
