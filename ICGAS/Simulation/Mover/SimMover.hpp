#pragma once
#include "pch.hpp"
#include "../../Core/Sim/DefPlatform.hpp"

class SimPlatform;

// Mover基类：只负责根据平台状态和能力边界推进运动
class SimMover
{
public:
	virtual ~SimMover() = default;
	virtual E_MOVER mover_type() const = 0;
};

std::unique_ptr<SimMover> make_sim_mover(E_MOVER type);
