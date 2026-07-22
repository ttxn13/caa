#ifndef DEF_DIS_PDU_HPP
#define DEF_DIS_PDU_HPP
#include "pch.hpp"

#pragma pack(push, 1)

constexpr int DIS_PDU_HEADER_LEN			= 12	;	// DIS 通用头部长度
constexpr int DIS_PDU_ENTITY_STATE_LEN		= 144	;	// PDU-1 Entity State 固定长度
constexpr int DIS_PDU_FIRE_LEN				= 96	;	// PDU-2 Fire 固定长度
constexpr int DIS_PDU_DETONATION_LEN		= 104	;	// PDU-3 Detonation 固定长度
constexpr int DIS_PDU_ELC_EMISSION_FIXED_LEN = 28	;	// PDU-23 Electromagnetic Emission 固定头长度

// ============================================================================
// DIS PDU 通用头部，固定 12 字节。
// 说明：以下结构体保存的是解析后的主机字节序数值，不建议直接 memcpy 后使用多字节字段
// ============================================================================
struct S_PDU_HEADER {
	quint8  protocol_version;   // Protocol Version	1 byte，DIS 协议版本号；AFSIM 常见输出为 5，对应 IEEE 1278.1-1995 系列。
	quint8  exercise_id;        // Exercise ID		1 byte，演习编号；用于区分同一网络中的不同仿真演习。
	quint8  pdu_type;           // PDU Type			1 byte，PDU 类型编号；例如 1 为 Entity State，2 为 Fire，3 为 Detonation，23 为 Electromagnetic Emission。
	quint8  protocol_family;    // Protocol Family	1 byte，协议族；例如 Entity Information 为 1，Warfare 为 2，Radio Communications 为 4，Distributed Emission Regeneration 为 6。
	quint32 timestamp;          // Timestamp		4 bytes，PDU 生成或发送时间戳；DIS 报文中为大端序。
	quint16 pdu_length;         // PDU Length		2 bytes，PDU 总长度，单位为 byte；DIS 报文中为大端序。
	quint8  pdu_status;         // PDU Status		1 byte，新版 DIS 中为 PDU 状态字段；旧版本中可能作为填充或未使用字段处理。
	quint8  padding;            // Padding			1 byte，填充字节，用于保持 PDU 头部对齐。
};

// ============================================================================
// DIS PDU-1 Entity State，实体状态 PDU，固定字段 144 字节。
// 作用：描述实体身份、阵营、实体类型、速度、位置、姿态、外观、航位推算参数、标识和能力。
// 注意：若 variable_parameter_count > 0，144 字节之后还会跟随可变参数记录。
// ============================================================================
struct S_PDU_ENTITY_STATE {
	S_PDU_HEADER header								;	// 通用 PDU 头部，12 bytes。

	struct {
		QString entity_identifier					;	// Entity Identifier，			6 bytes，实体唯一标识，由 Site ID、Application ID、Entity ID 组成；需一直继承传递。
		quint16 entity_id_site						;	// Entity ID - Site，			2 bytes，实体所属站点编号；DIS 实体唯一标识的第 1 段。
		quint16 entity_id_application				;	// Entity ID - Application，	2 bytes，实体所属应用编号；DIS 实体唯一标识的第 2 段。
		quint16 entity_id_entity					;	// Entity ID - Entity，			2 bytes，实体编号；DIS 实体唯一标识的第 3 段。
	}entity_id;

	quint8  force_id								;	// Force ID，					1 byte，阵营标识；需结合 dis_interface 中 force red/blue 映射解释。
	quint8  variable_parameter_count				;	// Number of Variable Parm，	1 byte，实体状态 PDU 后续可变参数记录数量。
	
	struct {
		QString entity_type_complete				;	// Entity Type，				8 bytes，实体类型由 7 个字段组成，通常表示为 "kind:domain:country:category:subcategory:specific:extra" 的字符串格式。	
		quint8  entity_type_kind					;	// Entity Type - Entity Kind，	1 byte，实体大类；例如 Platform、Munition 等。
		quint8  entity_type_domain					;	// Entity Type - Domain，		1 byte，实体作战域或运动域；例如 Land、Air、Surface、Subsurface、Space 等。
		quint16 entity_type_country					;	// Entity Type - Country，		2 bytes，国家或地区代码。
		quint8  entity_type_category				;	// Entity Type - Category，		1 byte，实体类别；例如战斗机、预警机、驱逐舰等类别。
		quint8  entity_type_subcategory				;	// Entity Type - Subcategory，	1 byte，实体子类别。
		quint8  entity_type_specific				;	// Entity Type - Specific，		1 byte，具体型号或细分类。
		quint8  entity_type_extra					;	// Entity Type - Extra，		1 byte，附加型号或扩展细分字段。
	}entity_type, alt_entity_type;
	
	float   entity_linear_velocity_x				;	// Entity Linear Velocity X，	4 bytes，实体在 ECEF 坐标系 X 方向速度，单位 m/s。
	float   entity_linear_velocity_y				;	// Entity Linear Velocity Y，	4 bytes，实体在 ECEF 坐标系 Y 方向速度，单位 m/s。
	float   entity_linear_velocity_z				;	// Entity Linear Velocity Z，	4 bytes，实体在 ECEF 坐标系 Z 方向速度，单位 m/s。
	double  entity_location_x						;	// Entity Location X，			8 bytes，实体在 ECEF 坐标系 X 坐标，单位 m。
	double  entity_location_y						;	// Entity Location Y，			8 bytes，实体在 ECEF 坐标系 Y 坐标，单位 m。
	double  entity_location_z						;	// Entity Location Z，			8 bytes，实体在 ECEF 坐标系 Z 坐标，单位 m。
	float   entity_orientation_psi					;	// Entity Orientation Psi，		4 bytes，实体姿态航向/偏航角，单位 rad。
	float   entity_orientation_theta				;	// Entity Orientation Theta，	4 bytes，实体姿态俯仰角，单位 rad。
	float   entity_orientation_phi					;	// Entity Orientation Phi，		4 bytes，实体姿态横滚角，单位 rad。
	quint32 entity_appearance						;	// Entity Appearance，			4 bytes，实体外观状态位；损伤、冒烟、运动状态等含义与实体类型相关。

	quint8  dead_reckoning_algorithm				;	// Dead Reckoning Algorithm，	1 byte，航位推算算法编号，用于接收端外推实体状态。
	quint8  dead_reckoning_other_parameters[15]		;	// Dead Reckoning Other parm，	15 bytes，航位推算附加参数。
	float   dead_reckoning_linear_acceleration_x	;	// DR Linear Acceleration X，	4 bytes，ECEF X 方向线加速度，单位 m/s^2。
	float   dead_reckoning_linear_acceleration_y	;	// DR Linear Acceleration Y，	4 bytes，ECEF Y 方向线加速度，单位 m/s^2。
	float   dead_reckoning_linear_acceleration_z	;	// DR Linear Acceleration Z，	4 bytes，ECEF Z 方向线加速度，单位 m/s^2。
	float   dead_reckoning_angular_velocity_x		;	// DR Angular Velocity X，		4 bytes，绕实体坐标系 X 轴角速度，单位 rad/s。
	float   dead_reckoning_angular_velocity_y		;	// DR Angular Velocity Y，		4 bytes，绕实体坐标系 Y 轴角速度，单位 rad/s。
	float   dead_reckoning_angular_velocity_z		;	// DR Angular Velocity Z，		4 bytes，绕实体坐标系 Z 轴角速度，单位 rad/s。

	quint8  entity_marking_character_set			;	// Entity Marking Character，	1 byte，实体标识字符串字符集。
	quint8  entity_marking[11]						;	// Entity Marking，				11 bytes，实体标识字符串；常用于平台短名称或显示标记。
	quint32 entity_capabilities						;	// Entity Capabilities，		4 bytes，实体能力标志位；可表达补给、恢复、修理等能力，具体位含义依 DIS 标准。
};

// ============================================================================
// DIS PDU-2 Fire，开火 PDU，固定 96 字节。
// 作用：表示某一实体发射武器的事件，包含发射者、目标、弹药、事件、发射位置、弹药描述、速度和射程。
// ============================================================================
struct S_PDU_FIRE {
	S_PDU_HEADER header									;	// 通用 PDU 头部，12 bytes。

	struct {
		QString entity_identifier						;	// Firing Entity ID，			6 bytes，发射实体唯一标识，由 Site ID、Application ID、Entity ID 组成。
		quint16 entity_id_site							;	// Firing Entity ID - Site，	2 bytes，发射实体所属站点编号。
		quint16 entity_id_application					;	// Firing Entity ID - Application，2 bytes，发射实体所属应用编号。
		quint16 entity_id_entity						;	// Firing Entity ID - Entity，	2 bytes，发射实体编号。
	}firing_entity_id;

	struct {
		QString entity_identifier						;	// Target Entity ID，			6 bytes，目标实体唯一标识；未知目标通常为 0:0:0。
		quint16 entity_id_site							;	// Target Entity ID - Site，	2 bytes，目标实体所属站点编号。
		quint16 entity_id_application					;	// Target Entity ID - Application，2 bytes，目标实体所属应用编号。
		quint16 entity_id_entity						;	// Target Entity ID - Entity，	2 bytes，目标实体编号。
	}target_entity_id;

	struct {
		QString entity_identifier						;	// Munition ID，				6 bytes，弹药或消耗品实体唯一标识。
		quint16 entity_id_site							;	// Munition ID - Site，		2 bytes，弹药实体所属站点编号。
		quint16 entity_id_application					;	// Munition ID - Application，2 bytes，弹药实体所属应用编号。
		quint16 entity_id_entity						;	// Munition ID - Entity，		2 bytes，弹药实体编号；显式武器通常会对应后续 Entity State。
	}munition_id;

	struct {
		QString event_identifier						;	// Event ID，					6 bytes，开火事件唯一标识，用于关联后续 Detonation PDU。
		quint16 event_id_site							;	// Event ID - Site，			2 bytes，开火事件所属站点编号。
		quint16 event_id_application					;	// Event ID - Application，	2 bytes，开火事件所属应用编号。
		quint16 event_id_event							;	// Event ID - Event，			2 bytes，开火事件编号。
	}event_id;

	quint32 fire_mission_index							;	// Fire Mission Index，		4 bytes，火力任务编号；用于关联同一火力任务中的多次开火。
	double  location_in_world_coordinate_x				;	// Location in World X，		8 bytes，发射点 ECEF X 坐标，单位 m。
	double  location_in_world_coordinate_y				;	// Location in World Y，		8 bytes，发射点 ECEF Y 坐标，单位 m。
	double  location_in_world_coordinate_z				;	// Location in World Z，		8 bytes，发射点 ECEF Z 坐标，单位 m。

	struct {
		QString entity_type_complete					;	// Burst Descriptor Entity Type，8 bytes，弹药实体类型完整字符串。
		quint8  entity_type_kind						;	// Entity Type - Entity Kind，1 byte，弹药实体大类。
		quint8  entity_type_domain						;	// Entity Type - Domain，		1 byte，弹药作战域或目标域。
		quint16 entity_type_country						;	// Entity Type - Country，		2 bytes，弹药国家或地区代码。
		quint8  entity_type_category					;	// Entity Type - Category，	1 byte，弹药类别。
		quint8  entity_type_subcategory					;	// Entity Type - Subcategory，1 byte，弹药子类别。
		quint8  entity_type_specific					;	// Entity Type - Specific，	1 byte，弹药具体型号或细分类。
		quint8  entity_type_extra						;	// Entity Type - Extra，		1 byte，弹药附加型号或扩展字段。
		quint16 warhead									;	// Warhead，					2 bytes，战斗部类型枚举。
		quint16 fuse									;	// Fuse，						2 bytes，引信类型枚举。
		quint16 quantity								;	// Quantity，					2 bytes，本次发射弹药数量。
		quint16 rate									;	// Rate，						2 bytes，射速或发射率。
	}burst_descriptor;

	float   velocity_x									;	// Velocity X，				4 bytes，弹药初始 ECEF X 方向速度，单位 m/s。
	float   velocity_y									;	// Velocity Y，				4 bytes，弹药初始 ECEF Y 方向速度，单位 m/s。
	float   velocity_z									;	// Velocity Z，				4 bytes，弹药初始 ECEF Z 方向速度，单位 m/s。
	float   range										;	// Range，					4 bytes，发射射程或目标距离，单位 m；未知时为 0。
};

// ============================================================================
// DIS PDU-3 Detonation，爆炸/终端事件 PDU，固定 104 字节。
// 作用：表示弹药终端阶段的爆炸、命中、近炸、未爆、脱靶或失效等结果。
// 注意：Detonation PDU 不等同于一定命中，需要结合 detonation_result 判断。
// ============================================================================
struct S_PDU_DETONATION {
	S_PDU_HEADER header									;	// 通用 PDU 头部，12 bytes。

	struct {
		QString entity_identifier						;	// Firing Entity ID，			6 bytes，原发射实体唯一标识。
		quint16 entity_id_site							;	// Firing Entity ID - Site，	2 bytes，原发射实体所属站点编号。
		quint16 entity_id_application					;	// Firing Entity ID - Application，2 bytes，原发射实体所属应用编号。
		quint16 entity_id_entity						;	// Firing Entity ID - Entity，	2 bytes，原发射实体编号。
	}firing_entity_id;

	struct {
		QString entity_identifier						;	// Target Entity ID，			6 bytes，目标实体唯一标识；未知目标通常为 0:0:0。
		quint16 entity_id_site							;	// Target Entity ID - Site，	2 bytes，目标实体所属站点编号。
		quint16 entity_id_application					;	// Target Entity ID - Application，2 bytes，目标实体所属应用编号。
		quint16 entity_id_entity						;	// Target Entity ID - Entity，	2 bytes，目标实体编号。
	}target_entity_id;

	struct {
		QString entity_identifier						;	// Munition ID，				6 bytes，弹药或消耗品实体唯一标识。
		quint16 entity_id_site							;	// Munition ID - Site，		2 bytes，弹药实体所属站点编号。
		quint16 entity_id_application					;	// Munition ID - Application，2 bytes，弹药实体所属应用编号。
		quint16 entity_id_entity						;	// Munition ID - Entity，		2 bytes，弹药实体编号。
	}munition_id;

	struct {
		QString event_identifier						;	// Event ID，					6 bytes，终端事件唯一标识，通常与 Fire PDU 的 Event ID 关联。
		quint16 event_id_site							;	// Event ID - Site，			2 bytes，终端事件所属站点编号。
		quint16 event_id_application					;	// Event ID - Application，	2 bytes，终端事件所属应用编号。
		quint16 event_id_event							;	// Event ID - Event，			2 bytes，终端事件编号。
	}event_id;

	float   velocity_x									;	// Velocity X，				4 bytes，弹药终端阶段 ECEF X 方向速度，单位 m/s。
	float   velocity_y									;	// Velocity Y，				4 bytes，弹药终端阶段 ECEF Y 方向速度，单位 m/s。
	float   velocity_z									;	// Velocity Z，				4 bytes，弹药终端阶段 ECEF Z 方向速度，单位 m/s。
	double  location_in_world_coordinate_x				;	// Location in World X，		8 bytes，爆炸/终端事件 ECEF X 坐标，单位 m。
	double  location_in_world_coordinate_y				;	// Location in World Y，		8 bytes，爆炸/终端事件 ECEF Y 坐标，单位 m。
	double  location_in_world_coordinate_z				;	// Location in World Z，		8 bytes，爆炸/终端事件 ECEF Z 坐标，单位 m。

	struct {
		QString entity_type_complete						;	// Burst Descriptor Entity Type，8 bytes，弹药实体类型完整字符串。
		quint8  entity_type_kind						;	// Entity Type - Entity Kind，1 byte，弹药实体大类。
		quint8  entity_type_domain						;	// Entity Type - Domain，		1 byte，弹药作战域或目标域。
		quint16 entity_type_country						;	// Entity Type - Country，		2 bytes，弹药国家或地区代码。
		quint8  entity_type_category					;	// Entity Type - Category，	1 byte，弹药类别。
		quint8  entity_type_subcategory					;	// Entity Type - Subcategory，1 byte，弹药子类别。
		quint8  entity_type_specific					;	// Entity Type - Specific，	1 byte，弹药具体型号或细分类。
		quint8  entity_type_extra						;	// Entity Type - Extra，		1 byte，弹药附加型号或扩展字段。
		quint16 warhead									;	// Warhead，					2 bytes，战斗部类型枚举。
		quint16 fuse									;	// Fuse，						2 bytes，引信类型枚举。
		quint16 quantity								;	// Quantity，					2 bytes，本次终端事件对应弹药数量。
		quint16 rate									;	// Rate，						2 bytes，射速或发射率。
	}burst_descriptor;

	float   location_in_entity_coordinate_x				;	// Location in Entity X，		4 bytes，爆炸点相对目标实体坐标系 X 坐标，单位 m。
	float   location_in_entity_coordinate_y				;	// Location in Entity Y，		4 bytes，爆炸点相对目标实体坐标系 Y 坐标，单位 m。
	float   location_in_entity_coordinate_z				;	// Location in Entity Z，		4 bytes，爆炸点相对目标实体坐标系 Z 坐标，单位 m。
	quint8  detonation_result							;	// Detonation Result，		1 byte，爆炸、命中、未爆、脱靶等终端结果枚举。
	quint8  variable_parameter_count					;	// Number of Variable Parm，	1 byte，后续可变参数记录数量。
	quint16 padding										;	// Padding，					2 bytes，填充字段。
};

// ============================================================================
// DIS PDU-23 Electromagnetic Emission，电磁辐射 PDU 固定部分，28 字节。
// 作用：描述雷达、火控雷达、主动雷达导引头、干扰机等电磁辐射系统状态。
// 注意：该 PDU 是变长 PDU，28 字节之后跟随 Emitter System Record、Beam Record 和 Track/Jam Target Record。
// ============================================================================
struct S_PDU_ELC_EMISSION {
	S_PDU_HEADER header									;	// 通用 PDU 头部，12 bytes。

	struct {
		QString entity_identifier						;	// Emitting Entity ID，		6 bytes，正在辐射实体唯一标识。
		quint16 entity_id_site							;	// Emitting Entity ID - Site，2 bytes，正在辐射实体的站点编号。
		quint16 entity_id_application					;	// Emitting Entity ID - Application，2 bytes，正在辐射实体的应用编号。
		quint16 entity_id_entity						;	// Emitting Entity ID - Entity，2 bytes，正在辐射实体的实体编号。
	}emitting_entity_id;

	struct {
		QString event_identifier							;	// Event ID，					6 bytes，本次电磁辐射事件唯一标识。
		quint16 event_id_site							;	// Event ID - Site，			2 bytes，本次电磁辐射事件所属站点编号。
		quint16 event_id_application					;	// Event ID - Application，	2 bytes，本次电磁辐射事件所属应用编号。
		quint16 event_id_event							;	// Event ID - Event，			2 bytes，本次电磁辐射事件编号。
	}event_id;

	quint8  state_update_indicator						;	// State Update Indicator，	1 byte，状态更新指示；0 为状态更新，1 为变化数据更新。
	quint8  number_of_systems							;	// Number of Systems，		1 byte，本 PDU 中包含的发射系统记录数量。
	quint16 padding										;	// Padding，					2 bytes，填充字段。
	QByteArray emitter_system_records					;	// Emitter System Records，	变长段原始字节；从 PDU 偏移 28 起按 number_of_systems 解析。

	struct S_TRACK_JAM_TARGET_RECORD {
		int start_offset								= 0	;	// Track/Jam Target Record 起始偏移。
		QString entity_identifier						;	// 被跟踪/干扰目标实体唯一标识。
		quint16 entity_id_site							= 0	;	// Target Entity ID - Site。
		quint16 entity_id_application					= 0	;	// Target Entity ID - Application。
		quint16 entity_id_entity						= 0	;	// Target Entity ID - Entity。
		quint8 emitter_number							= 0	;	// Emitter Number，目标关联的发射源编号。
		quint8 beam_number								= 0	;	// Beam Number，目标关联的波束编号。
	};

	struct S_BEAM_RECORD {
		int start_offset								= 0	;	// Beam Record 起始偏移。
		quint8 data_length								= 0	;	// Beam Data Length，单位为 32-bit word。
		quint8 beam_number								= 0	;	// Beam ID Number，波束编号。
		quint16 beam_parameter_index					= 0	;	// Beam Parameter Index。
		float fundamental_parameter_frequency			= 0.0f	;	// Frequency，频率，单位 Hz。
		float fundamental_parameter_frequency_range		= 0.0f	;	// Frequency Range，频率范围，单位 Hz。
		float fundamental_parameter_effective_radiated_power = 0.0f;	// Effective Radiated Power。
		float fundamental_parameter_pulse_repetition_frequency = 0.0f;	// Pulse Repetition Frequency。
		float fundamental_parameter_pulse_width			= 0.0f	;	// Pulse Width。
		float beam_data_azimuth_center					= 0.0f	;	// Azimuth Center，单位 rad。
		float beam_data_azimuth_sweep					= 0.0f	;	// Azimuth Sweep，单位 rad。
		float beam_data_elevation_center				= 0.0f	;	// Elevation Center，单位 rad。
		float beam_data_elevation_sweep					= 0.0f	;	// Elevation Sweep，单位 rad。
		float beam_data_sweep_sync						= 0.0f	;	// Sweep Sync。
		quint8 beam_function							= 0	;	// Beam Function。
		quint8 number_of_targets						= 0	;	// Number of Targets in Track/Jam Field。
		quint8 high_density_track_jam					= 0	;	// High Density Track/Jam。
		quint8 beam_status								= 0	;	// Beam Status。
		quint8 jamming_technique_kind					= 0	;	// Jamming Technique - Kind。
		quint8 jamming_technique_category				= 0	;	// Jamming Technique - Category。
		quint8 jamming_technique_subcategory			= 0	;	// Jamming Technique - Subcategory。
		quint8 jamming_technique_specific				= 0	;	// Jamming Technique - Specific。
		QByteArray extra_data							;	// Beam Record 声明长度之外的保留原始字节。
		std::vector<S_TRACK_JAM_TARGET_RECORD> target_records;
	};

	struct S_EMITTER_SYSTEM_RECORD {
		int start_offset								= 0	;	// Emitter System Record 起始偏移。
		quint8 data_length								= 0	;	// System Data Length，单位为 32-bit word。
		quint8 number_of_beams							= 0	;	// Number of Beams。
		quint16 padding									= 0	;	// Padding。
		quint16 emitter_name							= 0	;	// Emitter Name。
		quint8 emitter_function							= 0	;	// Emitter Function。
		quint8 emitter_number							= 0	;	// Emitter Number。
		float location_x								= 0.0f	;	// Location X，相对实体坐标，单位 m。
		float location_y								= 0.0f	;	// Location Y，相对实体坐标，单位 m。
		float location_z								= 0.0f	;	// Location Z，相对实体坐标，单位 m。
		QByteArray extra_data							;	// System Record 声明长度之外的保留原始字节。
		std::vector<S_BEAM_RECORD> beam_records			;
	};

	std::vector<S_EMITTER_SYSTEM_RECORD> record			;	// 已解析的发射系统记录。
	QStringList parse_warnings							;	// 变长段解析告警。

};

#pragma pack(pop)
#endif // DEF_DIS_PDU_HPP
