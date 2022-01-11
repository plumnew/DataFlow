// CDataFlow.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "pch.h"
#include <iostream>
#include "FlowBase.h"


class Source :public INode
{
public:
	OutPort<double> P1;
	OutPort<float> P2;

	Source(): 
		INode("Source", nodeType::nSource),
		P1("P1", "DOUBLE", PortType::POUT),
		P2("P2", "FLOAT", PortType::POUT)
	{

	}

	bool Configuration(string file) override
	{
		return false;
	}

	bool Excute() override
	{
		for (int i = 0; i < 100; i++)
		{
			double d = i;
			float f = i*0.1;
			P1.Submit(&d);
			P2.Submit(&f);
			Tout.TriggerNext();
			ResetTrigger();
		}
		return true;
	};
};

class Terminal :public INode
{
public:
	InPort<float> InP;

	Terminal() :
		INode("Terminal", nodeType::nTerminate),
		InP("P", "FLOAT", PortType::PIN) {

	}
	bool Configuration(string file) override
	{
		return false;
	}

	bool Excute() override
	{
		if (IsAllTriggerTrue())
		{
			printf("P:%f \r\n", *InP.objPoint);
		}
		return true;
	}
};

class Transfer :public INode
{
public:
	InPort<double> InP1;
	InPort<float> InP2;
	OutPort<float> OutP1;

	Transfer() :INode("Transfer", nodeType::nTransfer),
		InP1("P1", "DOUBLE", PortType::PIN),
		InP2("P2", "FLOAT", PortType::PIN),
		OutP1("P1", "FLOAT", PortType::POUT)
	{

	}
	bool Configuration(string file) override
	{
		return false;
	}

	bool Excute() override
	{
		if (IsAllTriggerTrue())
		{
			float c = (*InP1.objPoint) + (*InP2.objPoint);
			OutP1.Submit(&c);
			Tout.TriggerNext();
		}
		return true;
	}
};



static void TriggerEvent(string info)
{
	std::cout << info << std::endl;
}



int main()
{
	TriggerEvent::InfoLevel = Infolevel::none;
	TriggerEvent::OnTrigger = TriggerEvent;

	NodeGraphic ng;

	Source S;
	Transfer Tran;
	Terminal T;
	
	ng.addNode(S);
	ng.addNode(Tran);
	ng.addNode(T);

	ng.linkSignal(S.P1, Tran.InP1);
	ng.linkSignal(S.P2, Tran.InP2);
	ng.linkSignal(Tran.OutP1, T.InP);

	ng.connectNode(S, Tran);
	ng.connectNode(Tran, T);

	ng.start();

}
