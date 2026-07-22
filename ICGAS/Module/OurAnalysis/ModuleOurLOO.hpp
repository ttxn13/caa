#pragma once
#include "pch.hpp"
#include "../../Core/Model/DefEngage.hpp"

class ModuleOurLOO : public QObject
{
	Q_OBJECT
	friend class ModuleDatabase;

public:
	static ModuleOurLOO* GetInstance();
	static void DstInstance();

	void func_our_loo(
			  S_COA& coa_own,
		const std::map<QString, S_FORMAT>& list_own_fmt_align,
		const std::map<QString, S_FORMAT>& list_tgt_fmt_align,
		const std::map<QString, S_PLAT>& list_own_plat_align,
		const std::map<QString, S_PLAT>& list_tgt_plat_align);

private:
	explicit ModuleOurLOO(QObject* parent = nullptr);
	~ModuleOurLOO() override;

private:
	static QMutex mutex;
	static ModuleOurLOO* instance;
};
