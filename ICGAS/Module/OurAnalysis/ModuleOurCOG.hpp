#pragma once
#include "pch.hpp"
#include "../../Core/Model/DefEngage.hpp"

class ModuleOurCOG : public QObject
{
	Q_OBJECT
	friend class ModuleDatabase;

public:
	static ModuleOurCOG* GetInstance();
	static void DstInstance();

	void func_our_cog(
			  std::map<QString, S_FORMAT>& list_tgt_fmt_align,
		const std::map<QString, S_PLAT>& list_tgt_plat_align);

private:
	explicit ModuleOurCOG(QObject* parent = nullptr);
	~ModuleOurCOG() override;

private:
	static QMutex mutex;
	static ModuleOurCOG* instance;
};
