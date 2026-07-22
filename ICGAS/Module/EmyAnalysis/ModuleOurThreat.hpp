#pragma once
#include "pch.hpp"

class ModuleOurThreat : public QObject
{
	Q_OBJECT

public:
	static ModuleOurThreat* GetInstance();
	static void DstInstance();

private:
	explicit ModuleOurThreat(QObject* parent = nullptr);
	~ModuleOurThreat() override;

private:
	static QMutex mutex;
	static ModuleOurThreat* instance;
};
