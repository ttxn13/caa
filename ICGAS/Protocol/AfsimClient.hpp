#pragma once
#include "pch.hpp"
#include "../Core/IncCore.hpp"

class InfoExpert;
class QThread;
class QUdpSocket;

class AfsimClient : public QObject
{
	Q_OBJECT

public:		// 公有函数
	static AfsimClient*		GetInstance();
	static void				DstInstance();
	AfsimClient();
	~AfsimClient();

	bool start_afs_client()	;	// 开始通信
	void stop_afs_client()	;	// 结束通信
	bool start(const QString& bind_addr, quint16 bind_port);	// 按指定地址端口开始通信
	void stop() { stop_afs_client(); }							// 结束通信
	bool running() const { return is_running; }					// 运行状态

signals:
	void sig_update_entity_state(const S_PLAT& plat);	// 1  - PDU处理EntityState	实体状态
	void sig_afs_client_msg(const QString& info)	;	// 状态回告
	void sig_datagram_received(const QByteArray& data, const QString& peer, quint16 port, qint64 time_ms);

private slots:
	void on_start_thread()	;	// 线程内启动项 socket
	void on_stop_thread()	;	// 线程内停止项 socket
	void on_data_recv()		;	// 接收通信

private:	// 私有函数
	void deal_pdu_div(const QByteArray& data, const long long recv_time_ms)	;	// 根据PDU类型分发处理
	void flush_recv_summary(const long long cur_time_ms, const bool force = false);	// 汇总接收日志

private:	// 私有变量
	static QMutex mutex;
	static AfsimClient* instance;
	QThread* thread_main	= nullptr	;	// 主线程
	QThread* thread_self	= nullptr	;	// AfsimClient子线程

	InfoExpert* expert		= nullptr	;	// 专家知识
	QUdpSocket* socket_udp	= nullptr	;	// UDP socket

	QString bind_addr	= ""		;	// 客户端addr
	quint16 bind_port	= 0			;	// 客户端port
	int		iter_ms		= 100		;	// 接收线程间隔 ms，暂未使用
	bool	is_running	= false		;	// 运行状态
	std::map<int, int> recv_pdu_count;	// PDU计数
	std::map<int, int> recv_pdu_summary_count;	// PDU汇总计数
	long long last_summary_time_ms = 0;	// 上次汇总日志时间

private:
	// ---------- 工具函数 ----------
	QString cur_time_str()								const;	// 当前时间字符串
	QString to_hex_preview(const QByteArray& datagram)	const;	// 原始数据包输出

};
