#pragma once
#include "pch.hpp"

class ModuleOurGroup : public QObject
{
	Q_OBJECT

public:
	static ModuleOurGroup* GetInstance();
	static void DstInstance();

private:
	explicit ModuleOurGroup(QObject* parent = nullptr);
	~ModuleOurGroup() override;

private:
	static QMutex mutex;
	static ModuleOurGroup* instance;
};
