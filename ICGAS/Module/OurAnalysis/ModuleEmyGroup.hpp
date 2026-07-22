#pragma once
#include "pch.hpp"
#include "../../Core/Model/DefEngage.hpp"

class ModuleEmyGroup : public QObject
{
	Q_OBJECT
	friend class ModuleDatabase;

public:
	static ModuleEmyGroup* GetInstance();
	static void DstInstance();

	// ---------- 主函数 ----------
	void func_emy_group(
			  std::map<QString, S_FORMAT>	& list_own_fmt_align	,		std::map<QString, S_FORMAT>	& list_tgt_fmt_align	,
		const std::map<QString, S_PLAT>		& list_own_plat_align	, const std::map<QString, S_PLAT>	& list_tgt_plat_align	,
		const std::map<QString, S_WEAPON>	& list_own_weapon_align	, const std::map<QString, S_WEAPON>	& list_tgt_weapon_align	);

private:
	explicit ModuleEmyGroup(QObject* parent = nullptr);
	~ModuleEmyGroup() override;

	// ---------- 内部函数 ----------
	void inherit_fmt(		// 直接读取编队信息
			  std::map<QString, S_FORMAT>	& list_fmt_align	,
		const std::map<QString, S_PLAT>		& list_plat_align	,
		const E_SIDE side);

	void solve_tgt_fmt(		// 算法分群敌方编队信息
			  std::map<QString, S_FORMAT>	& list_tgt_fmt_align	,
		const std::map<QString, S_PLAT>		& list_tgt_plat_align	,
		const std::map<QString, S_WEAPON>	& list_tgt_weapon_align	,
		const E_SIDE side);

	// ---------- 工具函数 ----------
	// 判断平台ID列表是否一致
	bool same_plat_id_list(const std::vector<QString>& lhs, const std::vector<QString>& rhs) const;

	// 计算平均角度
	double average_angle_rad(const double sin_sum, const double cos_sum, const double fallback_rad, const bool norm_0_2pi) const;

	// 计算编队平均运动帧
	S_MOTION_FRAME average_motion_frame(const std::vector<const S_MOTION_FRAME*>& list_member_motion) const;

	// 写入编队运动帧并裁剪历史长度
	double format_range_m(const S_MOTION_FRAME& center_motion, const std::vector<const S_MOTION_FRAME*>& list_member_motion) const;
	void push_format_motion(S_FORMAT& format, const S_MOTION_FRAME& motion) const;

private:
	static QMutex mutex;
	static ModuleEmyGroup* instance;
};
