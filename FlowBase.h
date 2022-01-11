#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <objbase.h>

using namespace std;


#define MAX_LEN 128

enum Infolevel { none = 0, dataflow = 1, lib = 2, all = 3};

class TriggerEvent
{
public:
	static void(*OnTrigger)(string info);
	static int InfoLevel;
	static void RaiseTriggerEvent(string info)
	{
		if ((*OnTrigger) != nullptr)
			(*OnTrigger)(info);
	};
};

int TriggerEvent::InfoLevel;
void(*TriggerEvent::OnTrigger)(string info) = nullptr;

static string GetGuid()
{
	char szuuid[MAX_LEN] = { 0 };
	GUID guid;
	auto r = CoCreateGuid(&guid);
	sprintf_s(
		szuuid,
		sizeof(szuuid),
		"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);

	return std::string(szuuid);
};

enum class PortType{
	PIN=0,
	POUT=1,
	Error
};

class IPort {
public:
	string IName;
	string ITypeName;
	PortType IPortType;
	IPort(string _name, string _typename, PortType _porttype) {
		IName = _name;
		ITypeName = _typename;
		IPortType = _porttype;
	};
	bool isMatch(string _name, string _typename)
	{
		if (IName.compare(_name) == 0 && ITypeName.compare(_typename) == 0)
		{
			return true;
		}
		else
			return false;
	};
};

class INode;
class TriggerIn : IPort
{
public:
	vector<INode*> nodes;
	TriggerIn() :IPort("TRIGGER", "BOOL", PortType::PIN) {};
	~TriggerIn() { 
		nodes.clear();
		if (TriggerEvent::InfoLevel == Infolevel::lib)
		{
			ostringstream oss;
			oss << "Free TriggerIn:" << IName;
			TriggerEvent::RaiseTriggerEvent(oss.str());
		}

	};
};


class TriggerOut : IPort
{
public:
	uint8_t trigger = 0x0;
	TriggerOut() :IPort("TRIGGER", "BOOL", PortType::POUT) {};
	~TriggerOut()
	{
		nodes.clear();
		if (TriggerEvent::InfoLevel == Infolevel::lib)
		{
			ostringstream oss;
			oss << "Free TriggerOut:" << IName;
			TriggerEvent::RaiseTriggerEvent(oss.str());
		}

	}
	void ConnectTo(INode* _current, INode* _node);
	void TriggerNext(bool IsParralle = false);
private:
	vector<INode*> nodes;
	string nodeID = "";
};

enum class nodeType: uint8_t { nSource, nTransfer, nTerminate };

class INode
{
public:
	string IName;
	TriggerIn Tin;
	TriggerOut Tout;
	string ID;
	nodeType NodeType;

	INode(string _name, nodeType _nodeType) : IName(_name), Tin(), Tout(), NodeType(_nodeType)
	{
		ID = GetGuid();
	};
	~INode();

	void LinkTo(INode* nextNode)
	{
		Tout.ConnectTo(this, nextNode);
	};

	INode& operator>>(INode& nextNode)
	{
		Tout.ConnectTo(this, &nextNode);
		return nextNode;
	}

	bool IsAllTriggerTrue()
	{
		auto s = find_if(Tin.nodes.begin(), Tin.nodes.end(), [](INode* node) { return node->Tout.trigger == 0; });
		if (s == Tin.nodes.end())
			return true;
		else
			return false;
	}

	bool IsAnyTriggerTrue()
	{
		auto s = find_if(Tin.nodes.begin(), Tin.nodes.end(), [](INode* node) { return node->Tout.trigger > 0; });
		if (s != Tin.nodes.end())
			return true;
		else
			return false;
	}

	bool IsUserTriggerTrue(uint8_t code)
	{
		auto s = find_if(Tin.nodes.begin(), Tin.nodes.end(), [code](INode* node) { return node->Tout.trigger == code; });
		if (s != Tin.nodes.end())
			return true;
		else
			return false;
	}

	void ResetTrigger()
	{
		Tout.trigger = 0;
		for(auto it = Tin.nodes.begin(); it!= Tin.nodes.end(); it++)
		{
			(*it)->ResetTrigger();
		}
	};
	virtual bool Excute() = 0;
	virtual bool Configuration(string file) = 0;
};

inline INode::~INode() { 
	if (TriggerEvent::InfoLevel == Infolevel::lib)
	{
		ostringstream oss;
		oss << "Free Node:" << IName;
		TriggerEvent::RaiseTriggerEvent(oss.str());
	}
};

inline void TriggerOut::ConnectTo(INode* _current, INode* _node) {
	_node->Tin.nodes.push_back(_current);
	nodes.push_back(_node);
	nodeID = _current->ID;
};

inline void TriggerOut::TriggerNext(bool IsParralle)
{
	trigger = true;
	if (TriggerEvent::InfoLevel == Infolevel::dataflow)
	{
		ostringstream oss;
		oss << "Node:" << nodeID;
		TriggerEvent::RaiseTriggerEvent(oss.str());
	}

	if (IsParralle == false)
	{
		for(auto it = nodes.begin(); it!= nodes.end(); it++)
		{
			(*it)->Excute();
		}
	}
};

template<typename T>
class InPort :IPort
{
public:
	T* objPoint = nullptr;
	InPort(string _name, string _typename, PortType _porttype) :IPort(_name, _typename, _porttype)
	{

	};
	~InPort() {
		objPoint = nullptr;
		if (TriggerEvent::InfoLevel == Infolevel::lib)
		{
			ostringstream oss;
			oss << "Free InPort:" << IName;
			TriggerEvent::RaiseTriggerEvent(oss.str());
		}

	};
};

template<typename T>
class OutPort :IPort
{
public:
	OutPort(string _name, string _typename, PortType _porttype) :IPort(_name, _typename, _porttype)
	{

	};
	~OutPort() {
		delete node;
		ports.clear();
		if (TriggerEvent::InfoLevel == Infolevel::lib)
		{
			ostringstream oss;
			oss << "Free OutPort:" << IName;
			TriggerEvent::RaiseTriggerEvent(oss.str());
		}

	};
	void ConnectTo(InPort<T>* _port) {
		ports.push_back(_port);
	};

	void operator>(InPort<T>& _port) {
		ports.push_back(&_port);
	};

	void Submit(T* t, bool update = true) 
	{
		for (auto it = ports.begin(); it!= ports.end(); it++)
		{
			(*it)->objPoint = t;
		}
	};
private:
	INode* node = nullptr;;
	vector<InPort<T>*> ports;
};

union timestamp
{
	uint64_t t_u64;
	int64_t t_i64;
};

class NodeGraphic
{
public:
	void addNode(INode &node) {
		nodes.push_back(&node);
	};
	template<typename T>
	void linkSignal(OutPort<T> &outport, InPort<T> &inport) {
		outport > inport;
	};

	void connectNode(INode &node1, INode &node2)
	{
		node1 >> node2;
	};

	void start()
	{
		std::vector<INode*> s_node;
		for (auto iter = nodes.begin(); iter != nodes.end(); iter++)
		{
			if((*iter)->NodeType == nodeType::nSource)
				s_node.push_back((*iter));
		}

		while (none_of(s_node.begin(), s_node.end(), [&](INode* node) { return node->Excute(); }))
		{
			
		}
	}
private:
	std::vector<INode*> nodes;
};
