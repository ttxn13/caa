#pragma once
#include "pch.hpp"

class ModuleOurCOA : public QObject
{
	Q_OBJECT

public:
	static ModuleOurCOA* GetInstance();
	static void DstInstance();

private:
	explicit ModuleOurCOA(QObject* parent = nullptr);
	~ModuleOurCOA() override;

private:
	static QMutex mutex;
	static ModuleOurCOA* instance;
};
