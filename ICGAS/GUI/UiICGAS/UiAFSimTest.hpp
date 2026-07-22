#pragma once
#include "pch.hpp"
#include "../../Protocol/AfsimClient.hpp"
#include "../../Protocol/AfsimServer.hpp"

struct S_AFSimTestDecodeResult
{
	bool ok = false;
	int pdu_type = -1;
	QString type_name;
	QString structure_name;
	QString summary;
	QString error;
	QVariantList fields;
	QVariantMap structure;
	QVariantMap row;
};

struct S_AFSimTestLogStore
{
	QStringList rows;

	S_AFSimTestLogStore();
	bool append(const QString& info);
	void clear();

private:
	QString merge_key(const QString& info) const;
};

struct S_AFSimTestPduViews
{
	QVariantList entity_rows;
	QVariantList fire_rows;
	QVariantList detonation_rows;
	QVariantList emission_rows;
	QVariantMap latest_entity_state;
	QVariantMap latest_fire;
	QVariantMap latest_detonation;
	QVariantMap latest_emission;
	QString entity_summary = QStringLiteral("尚未收到实体状态 PDU。");
	QString fire_summary = QStringLiteral("尚未收到发射 PDU。");
	QString detonation_summary = QStringLiteral("尚未收到爆炸 PDU。");
	QString emission_summary = QStringLiteral("尚未收到电磁辐射 PDU。");

	void clear();
	bool update(const S_AFSimTestDecodeResult& decoded);
};

struct S_AFSimTestPacketUpdate
{
	bool rows_changed = false;
	bool selection_changed = false;
	bool pdu_views_changed = false;
};

struct S_AFSimTestPacketStore
{
	QVariantList rows;
	QVariantList selected_rows;
	QString selected_hex;
	QString selected_summary = QStringLiteral("未选择数据包。");
	QString selected_structure_name = QStringLiteral("-");
	int selected_index = -1;
	int seq = 0;
	S_AFSimTestPduViews pdu_views;

	void clear();
	bool select(int index);
	S_AFSimTestPacketUpdate append(const QByteArray& data, const QString& direction,
		const QString& endpoint, qint64 time_ms);
};

struct S_AFSimTestEntityIdentifier
{
	QString text;
	quint16 site = 0;
	quint16 application = 0;
	quint16 entity = 0;
};

struct S_AFSimTestEntityType
{
	QString text;
	quint8 kind = 0;
	quint8 domain = 0;
	quint16 country = 0;
	quint8 category = 0;
	quint8 subcategory = 0;
	quint8 specific = 0;
	quint8 extra = 0;
};

struct S_AFSimTestBurstDescriptor
{
	S_AFSimTestEntityType entity_type;
	quint16 warhead = 0;
	quint16 fuse = 0;
	quint16 quantity = 0;
	quint16 rate = 0;
};

class UiAFSimTestSendModel
{
public:
	UiAFSimTestSendModel();

	QStringList pdu_types = {
		QStringLiteral("PDU-01 实体状态"),
		QStringLiteral("PDU-02 发射"),
		QStringLiteral("PDU-03 爆炸"),
		QStringLiteral("PDU-23 电磁辐射")
	};
	int pdu_index = 0;
	QVariantList field_rows;
	QString hex;
	QString status = QStringLiteral("编辑字段，构建后发送。");

	bool set_pdu_index(int index);
	bool set_hex(const QString& value);
	bool set_field_value(int index, const QString& value);
	bool set_field_value_by_key(const QString& key, const QString& value);
	bool apply_default_packet(const QByteArray& data);
	bool clear_default_packet();
	bool can_apply_received_default() const { return !manual_override; }
	bool using_default_packet() const { return default_packet_active && !manual_override; }
	int current_pdu_type() const;
	void rebuild_fields();
	QByteArray build_packet(QString* error) const;

private:
	void set_field_value_quiet(const QString& key, const QString& value);
	void sync_entity_velocity_fields(const QString& changed_key);
	void apply_id_defaults(const QString& prefix, const S_AFSimTestEntityIdentifier& id);
	void apply_type_defaults(const QString& prefix, const S_AFSimTestEntityType& type);
	void apply_burst_defaults(const S_AFSimTestBurstDescriptor& burst);
	QString field_value(const QString& key) const;
	quint8 field_u8(const QString& key, quint8 fallback = 0) const;
	quint16 field_u16(const QString& key, quint16 fallback = 0) const;
	quint32 field_u32(const QString& key, quint32 fallback = 0) const;
	float field_f32(const QString& key, float fallback = 0.0f) const;
	double field_f64(const QString& key, double fallback = 0.0) const;
	S_AFSimTestEntityIdentifier field_id(const QString& prefix) const;
	S_AFSimTestEntityType field_type(const QString& prefix) const;
	S_AFSimTestBurstDescriptor field_burst(const QString& prefix) const;

	QByteArray default_packet;
	quint8 entity_exercise_id = 1;
	quint8 entity_force_id = 1;
	bool default_packet_active = false;
	bool manual_override = false;
};

class UiAFSimTest : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString bindAddr READ bindAddr WRITE setBindAddr NOTIFY configChanged)
	Q_PROPERTY(QString bindPort READ bindPort WRITE setBindPort NOTIFY configChanged)
	Q_PROPERTY(QString targetAddr READ targetAddr WRITE setTargetAddr NOTIFY configChanged)
	Q_PROPERTY(QString targetPort READ targetPort WRITE setTargetPort NOTIFY configChanged)
	Q_PROPERTY(bool clientRunning READ clientRunning NOTIFY stateChanged)
	Q_PROPERTY(bool serverRunning READ serverRunning NOTIFY stateChanged)
	Q_PROPERTY(QStringList logRows READ logRows NOTIFY logRowsChanged)
	Q_PROPERTY(QVariantList packetRows READ packetRows NOTIFY packetRowsChanged)
	Q_PROPERTY(QVariantList selectedPacketRows READ selectedPacketRows NOTIFY selectedPacketChanged)
	Q_PROPERTY(int selectedPacketIndex READ selectedPacketIndex NOTIFY selectedPacketChanged)
	Q_PROPERTY(QString selectedHex READ selectedHex NOTIFY selectedPacketChanged)
	Q_PROPERTY(QString selectedSummary READ selectedSummary NOTIFY selectedPacketChanged)
	Q_PROPERTY(QString selectedStructureName READ selectedStructureName NOTIFY selectedPacketChanged)
	Q_PROPERTY(QVariantList entityRows READ entityRows NOTIFY pduViewsChanged)
	Q_PROPERTY(QVariantList fireRows READ fireRows NOTIFY pduViewsChanged)
	Q_PROPERTY(QVariantList detonationRows READ detonationRows NOTIFY pduViewsChanged)
	Q_PROPERTY(QVariantList emissionRows READ emissionRows NOTIFY pduViewsChanged)
	Q_PROPERTY(QString entitySummary READ entitySummary NOTIFY pduViewsChanged)
	Q_PROPERTY(QString fireSummary READ fireSummary NOTIFY pduViewsChanged)
	Q_PROPERTY(QString detonationSummary READ detonationSummary NOTIFY pduViewsChanged)
	Q_PROPERTY(QString emissionSummary READ emissionSummary NOTIFY pduViewsChanged)
	Q_PROPERTY(QStringList sendPduTypes READ sendPduTypes CONSTANT)
	Q_PROPERTY(int sendPduIndex READ sendPduIndex WRITE setSendPduIndex NOTIFY sendModelChanged)
	Q_PROPERTY(QVariantList sendFieldRows READ sendFieldRows NOTIFY sendModelChanged)
	Q_PROPERTY(QString sendHex READ sendHex WRITE setSendHex NOTIFY sendModelChanged)
	Q_PROPERTY(QString sendStatus READ sendStatus NOTIFY sendModelChanged)

public:
	explicit UiAFSimTest(QObject* parent = nullptr);
	~UiAFSimTest();

	void show();
	void setWindowIcon(const QIcon& icon);

	QString bindAddr() const { return bind_addr; }
	QString bindPort() const { return bind_port; }
	QString targetAddr() const { return target_addr; }
	QString targetPort() const { return target_port; }
	bool clientRunning() const { return client.running(); }
	bool serverRunning() const { return server.running(); }
	QStringList logRows() const { return log_store.rows; }
	QVariantList packetRows() const { return packet_store.rows; }
	QVariantList selectedPacketRows() const { return packet_store.selected_rows; }
	int selectedPacketIndex() const { return packet_store.selected_index; }
	QString selectedHex() const { return packet_store.selected_hex; }
	QString selectedSummary() const { return packet_store.selected_summary; }
	QString selectedStructureName() const { return packet_store.selected_structure_name; }
	QVariantList entityRows() const { return packet_store.pdu_views.entity_rows; }
	QVariantList fireRows() const { return packet_store.pdu_views.fire_rows; }
	QVariantList detonationRows() const { return packet_store.pdu_views.detonation_rows; }
	QVariantList emissionRows() const { return packet_store.pdu_views.emission_rows; }
	QString entitySummary() const { return packet_store.pdu_views.entity_summary; }
	QString fireSummary() const { return packet_store.pdu_views.fire_summary; }
	QString detonationSummary() const { return packet_store.pdu_views.detonation_summary; }
	QString emissionSummary() const { return packet_store.pdu_views.emission_summary; }
	QStringList sendPduTypes() const { return send_model.pdu_types; }
	int sendPduIndex() const { return send_model.pdu_index; }
	QVariantList sendFieldRows() const { return send_model.field_rows; }
	QString sendHex() const { return send_model.hex; }
	QString sendStatus() const { return send_model.status; }

	void setBindAddr(const QString& value);
	void setBindPort(const QString& value);
	void setTargetAddr(const QString& value);
	void setTargetPort(const QString& value);
	void setSendPduIndex(int index);
	void setSendHex(const QString& value);

	Q_INVOKABLE void toggleClient();
	Q_INVOKABLE void toggleServer();
	Q_INVOKABLE void selectPacket(int index);
	Q_INVOKABLE void setSendFieldValue(int index, const QString& value);
	Q_INVOKABLE void setSendFieldValueByKey(const QString& key, const QString& value);
	Q_INVOKABLE void buildSendPacket();
	Q_INVOKABLE void sendStructuredPacket();
	Q_INVOKABLE void decodeSendHex();
	Q_INVOKABLE void sendHexPacket();
	Q_INVOKABLE void clearLogs();
	Q_INVOKABLE void clearPackets();
	Q_INVOKABLE void release();
	Q_INVOKABLE void closeWindow();

signals:
	void configChanged();
	void stateChanged();
	void logRowsChanged();
	void packetRowsChanged();
	void selectedPacketChanged();
	void pduViewsChanged();
	void sendModelChanged();

private slots:
	void on_client_log(const QString& info);
	void on_server_log(const QString& info);
	void on_datagram_received(const QByteArray& data, const QString& peer, quint16 port, qint64 time_ms);
	void on_datagram_sent(const QByteArray& data, const QString& target, quint16 port, qint64 time_ms);

private:
	QObject* root_object() const;
	void center_on_primary_screen(QQuickWindow* window);
	void load_icgas_comm_config();
	void append_log(const QString& info);
	void append_packet(const QByteArray& data, const QString& direction, const QString& endpoint, qint64 time_ms);
	bool start_server(bool send_startup_platform);
	bool send_startup_platform_state();
	bool remember_first_received_packet(const QByteArray& data, const QString& direction);
	bool apply_first_received_default();
	bool parse_port(const QString& text, quint16& port, const QString& name);
	QString cur_time_str() const;

	QQmlApplicationEngine engine;
	QIcon window_icon;
	AfsimClient client;
	AfsimServer server;
	S_AFSimTestLogStore log_store;
	S_AFSimTestPacketStore packet_store;
	UiAFSimTestSendModel send_model;
	std::map<int, QByteArray> first_received_packets;
	QByteArray startup_entity_state_packet;

	QString bind_addr = QStringLiteral("0.0.0.0");
	QString bind_port = QStringLiteral("8888");
	QString target_addr = QStringLiteral("192.168.50.199");
	QString target_port = QStringLiteral("9999");
	bool closing = false;
};
