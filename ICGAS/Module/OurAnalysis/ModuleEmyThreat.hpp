#pragma once
#include "pch.hpp"
#include "../../Core/Model/DefEngage.hpp"

class ModuleEmyThreat : public QObject
{
	Q_OBJECT
	friend class ModuleDatabase;

public:
	static ModuleEmyThreat* GetInstance();
	static void DstInstance();

	void func_emy_threat(
			  std::map<QString, S_FORMAT>& list_own_fmt_align,
			  std::map<QString, S_FORMAT>& list_tgt_fmt_align,
			  std::map<QString, S_PLAT>& list_own_plat_align,
			  std::map<QString, S_PLAT>& list_tgt_plat_align);

private:
	explicit ModuleEmyThreat(QObject* parent = nullptr);
	~ModuleEmyThreat() override;

private:
	static QMutex mutex;
	static ModuleEmyThreat* instance;
};
