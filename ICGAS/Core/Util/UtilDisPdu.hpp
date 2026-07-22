#pragma once
#include "pch.hpp"
#include "../Comm/CoreGraph.hpp"
#include "../Comm/CoreDefine.hpp"
#include "../Model/DefEngage.hpp"
#include "../Model/DefDisPdu.hpp"
#include "UtilCoor.hpp"

namespace UtilDisPdu {
	// ---------- 基本数据读取，大端序 ----------
	quint8  read_u8(const QByteArray& data, int offset);
	quint16 read_u16(const QByteArray& data, int offset);
	quint32 read_u32(const QByteArray& data, int offset);
	quint64 read_u64(const QByteArray& data, int offset);
	float   read_f32(const QByteArray& data, int offset);
	double  read_f64(const QByteArray& data, int offset);

	// ---------- 通用字段转换 ----------
	QString entity_identifier_string(const QByteArray& data, int offset);
	QString entity_type_string(const QByteArray& data, int offset);

	// ---------- DIS PDU 解析 ----------
	void recv_pdu_header		(const QByteArray& data, S_PDU_HEADER& header);		// DIS PDU--  PDU头解析
	bool deal_pdu_entity_state	(const QByteArray& data, S_PDU_ENTITY_STATE& pdu);	// DIS PDU-1  实体状态解析
	//bool deal_pdu_fire			(const QByteArray& data, S_PDU_FIRE& pdu);			// DIS PDU-2  开火解析
	//bool deal_pdu_detonation	(const QByteArray& data, S_PDU_DETONATION& pdu);	// DIS PDU-3  爆炸解析
	//bool deal_pdu_elc_emissio	(const QByteArray& data, S_PDU_ELC_EMISSION& pdu);	// DIS PDU-23 电磁辐射解析
	
	// ---------- DIS 映射解析 ----------
	bool deal_pdu_entity_state(const QByteArray& data, S_PLAT& plat);	// DIS PDU-1  实体状态解析并映射到平台

	// ---------- 项目业务映射 ----------
	QString		get_dis_ori_id	(const QByteArray& data, int offset);
	QString		get_dis_ori_type(const QByteArray& data, int offset);
	E_SIDE get_dis_side	(const QByteArray& data, int offset);
	E_PLAT_TYPE get_dis_type	(const QByteArray& data, int offset);

}
