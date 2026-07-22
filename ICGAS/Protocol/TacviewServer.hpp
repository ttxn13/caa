#pragma once
#include "pch.hpp"
#include "../Core/IncCore.hpp"

class TacviewServer : public QObject
{
	Q_OBJECT

public:		// 公有函数
	static TacviewServer*	GetInstance();
	static void				DstInstance();
	TacviewServer();
	~TacviewServer();

	bool start_tac_server()	; 	// 开始Tacview实时遥测服务
	void stop_tac_server()	; 	// 停止Tacview实时遥测服务
	bool running() const		{ return is_running; }

signals:
	void sig_tac_server_msg(const QString& info);

private slots:
	void on_start_thread()			; 	// 线程内启动项 socket和timer
	void on_stop_thread()			; 	// 线程内停止项 socket和timer
	void on_new_connection()		; 	// Tacview连接
	void on_client_ready_read()		; 	// Tacview握手回包
	void on_client_disconnected()	; 	// Tacview断开
	void on_timer_send()			; 	// 定时发送态势

private:	// 私有函数
	bool init_config()			;	// 读取并校验Tacview配置
	void clear_client()			;	// 清理Tacview客户端连接
	void clear_server()			;	// 清理Tcp服务端
	void clear_timer()			;	// 清理发送定时器
	void send_handshake()		;	// 发送Tacview握手
	void send_acmi_header()		;	// 发送ACMI头

	QString cur_time_str()							const; 	// 当前时间字符串
	QString tac_color(const E_SIDE side)		const; 	// Tacview颜色
	QString tac_type(const E_PLAT_TYPE type)		const; 	// Tacview类型
	QString tac_platform_type_code(const E_PLAT_TYPE type) const;	// 平台类型代号
	QString tac_model_name(const S_PLAT& plat)		const;	// Tacview平台型号
	QString tac_weapon_type(const E_WEAPON_TYPE type) const;	// Tacview武器类型
	QString tac_weapon_type_code(const E_WEAPON_TYPE type) const;	// 武器类型代号
	QString tac_model_name(const S_WEAPON& weapon) const;	// Tacview武器型号
	QString tac_shape(const S_PLAT& plat)			const;	// Tacview三维模型
	QString tac_text(const QString& value)			const;	// Tacview文本转义
	double tac_pitch_deg(const S_PLAT& plat)			const;	// Tacview俯仰角
	bool active_tac_object(const S_PLAT& plat)		const;	// 是否应在Tacview中显示
	bool active_tac_object(const S_WEAPON& weapon)	const;	// 武器是否应在Tacview中显示
	QString object_name(const S_PLAT& plat)			const; 	// Tacview对象名称
	QString object_id(const S_PLAT& plat)			const; 	// Tacview对象ID
	QString object_id(const S_WEAPON& weapon)		const; 	// Tacview武器对象ID
	QString object_label(const S_PLAT& plat)			const; 	// Tacview显示标签
	QString object_label(const S_WEAPON& weapon)		const; 	// Tacview武器显示标签
	QString hashed_object_id(const QString& raw_id, quint32 type_prefix) const;	// 字符串对象ID转Tacview数字ID
	void append_object_frame(QByteArray& frame, const S_PLAT& plat) const;	// 追加Tacview对象帧
	void append_object_frame(QByteArray& frame, const S_WEAPON& weapon) const;	// 追加Tacview武器对象帧
	void append_remove_object_frame(QByteArray& frame, const QString& id) const;	// 追加Tacview对象移除帧
	QHostAddress host_addr(const QString& addr)		const; 	// 字符串转IP

private:	// 私有变量
	static QMutex mutex;
	static TacviewServer* instance;

	QThread* thread_main	= nullptr	; 	// 主线程
	QThread* thread_self	= nullptr	; 	// TacviewServer子线程

	InfoExpert* expert		= nullptr	; 	// 专家知识
	ModuleDatabase* module_db	= nullptr	; 	// 态势数据库
	QTcpServer* server_tcp	= nullptr	; 	// Tacview TCP服务端
	QTcpSocket* client_tcp	= nullptr	; 	// Tacview连接客户端
	QTimer* timer_send		= nullptr	; 	// 定时发送态势

	QString bind_addr	= ""	;	// Tacview服务IP
	QString bind_user	= ""	;	// Tacview用户名
	quint16 bind_port	= 0		;	// Tacview服务端口
	int iter_ms			= 100	;	// 发送间隔 ms
	int frame_id		= 0		;	// Tacview帧号
	bool is_running		= false	;	// 运行状态
	bool is_connected	= false	;	// Tacview连接状态
	QDateTime start_time		;	// Tacview参考时间
	std::set<QString> active_tac_object_ids;	// 上一帧已发送到Tacview的对象ID
};
