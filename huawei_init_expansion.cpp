#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <cstring>
#include <sstream>
using namespace std;

// 提交
#define UPLOAD

//#define TEST

// 服务器能耗与性价比存储
unordered_map<int, string> serverCPR_info;
// 服务器信息
unordered_map<string, vector<int>> serverInfos;
// 虚拟机信息
unordered_map<string, vector<int>> vmInfos;
// 请求信息
vector<vector<string>> requestInfos;
//存放购买的每次扩容时：服务器类型与对应的数量：重新映射ID。
unordered_map<int, string> Total_Server_ID;    //类型决定ID    key=主机名+购买时间+","+购买顺序
//unordered_map<string, int> Total_Server_NameID;    //存放购买的服务器类型与对应的数量：重新映射ID
//unordered_map<string, int>    ServerTypeBuyOrder;    //记录相同类型服务器的购买顺序，先购买的优先编号：从1开始
vector<string> day_BuyServers_res;                 //记录每天购买服务器的信息：主机名+购买时间+","+购买台数
// 购买的服务器信息
int serverNumber = 0;
int PreserverNumber = 0;
unordered_map<int, vector<int>> sysServerResource;
unordered_map<int, vector<int>> PresysServerResource;

// 当前开机服务器
vector<int> serverRunVms;
// 记录虚拟机运行在那个服务器上
unordered_map<string, vector<int>> vmOnServer;
unordered_map<string, vector<int>> PrevmOnServer;

vector<string> res;
int  dayNum = 0;
//所有虚拟机上Core和Mem的最大值
int  VM_max_Core = 0, VM_max_Mem = 0;
//双节点虚拟机所在服务器的信息：服务器ID， Key:部署各种节点 虚拟机的个数  key[0]双节点，key[1]单节点，
unordered_map<int, vector<int>>  NodeOnServerInfo;
unordered_map<int, vector<int>>  PreNodeOnServerInfo;
int    PredayServerNum = 0;


#ifdef TEST
const string filePath = "./training-data/training-1.txt";
#endif
// 成本
long long SERVERCOST = 0, POWERCOST = 0, TOTALCOST = 0;

// 按照服务器A点内存降序排序
// std::sort(serverInfos.begin(),serverInfos.end(),cmpServerMemoryDown)
// 按照服务器A点内存升序排序
//bool cmpServerMemoryUp(const Server &server1, const Server &server2)
//{
//    return server1.memoryA>server2.memoryA;
//}







/*@@@@@@@@@@@@@@@@@@@@@@@@@@@ add here @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/*
解析txt文件时，将可供购买的服务器类型信息解析保存
(型号，cpu，内存大小，硬件成本，每日能耗成本)
*/
struct _server_info_stu {
	string serverType;
	string cpuCores;
	string memorySize;
	string serverCost;
	string powerCost;
};

/*
解析txt文件时，将可售卖虚拟机类型信息解析保存
(型号，cpu核数，内存大小，是否双节点部署)
*/
struct _vm_info_stu {
	string vmType;
	string vmCpuCores;
	string vmMemory;
	string vmTwoNodes;
};

struct _generate_requestAdd_info {
	string op;
	string reqVmType;
	string reqId;
};

struct _generate_requestDel_info {
	string op;
	string reqId;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@ end @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/






//添加了服务器的Core与Mem的比值
void generateServer(_server_info_stu *server_info_stu)
{
	string _serverType = "";
	for (int i = 1; i<server_info_stu->serverType.size() - 1; i++) {
		_serverType += server_info_stu->serverType[i];
	}

	int _cpuCores = 0,
		_memorySize = 0,
		_serverCost = 0,
		_powerCost = 0,
		_Node_On_ServerInit=0,
		_serverCPR = 0;

	for (int i = 0; i<server_info_stu->cpuCores.size() - 1; i++)
	{
		_cpuCores = 10 * _cpuCores + server_info_stu->cpuCores[i] - '0';
	}

	for (int i = 0; i<server_info_stu->memorySize.size() - 1; i++)
	{
		_memorySize = 10 * _memorySize + server_info_stu->memorySize[i] - '0';
	}

	for (int i = 0; i<server_info_stu->serverCost.size() - 1; i++)
	{
		_serverCost = 10 * _serverCost + server_info_stu->serverCost[i] - '0';
	}

	for (int i = 0; i<server_info_stu->powerCost.size() - 1; i++)
	{
		_powerCost = 10 * _powerCost + server_info_stu->powerCost[i] - '0';
	}

	_serverCPR =  1000*(_serverCost + _powerCost) / (_memorySize);
	serverInfos[_serverType] = vector<int>{ _cpuCores / 2,
		_cpuCores / 2,
		_memorySize / 2,
		_memorySize / 2,
		_serverCost,
		_powerCost,
		_Node_On_ServerInit
	};
	serverCPR_info[_serverCPR] = string{ _serverType };
}

/*  解析txt文件时，将可售卖虚拟机类型信息解析保存
(型号，cpu核数，内存大小，是否双节点部署)
*/
void generateVm(_vm_info_stu *vm_info_stu)
{
	string _vmType;

	for (int i = 1; i<vm_info_stu->vmType.size() - 1; i++) {
		_vmType += vm_info_stu->vmType[i];
	}

	int _vmCpuCores = 0, _vmMemory = 0, _vmTwoNodes = 0;
	for (int i = 0; i<vm_info_stu->vmCpuCores.size() - 1; i++) {
		_vmCpuCores = _vmCpuCores * 10 + vm_info_stu->vmCpuCores[i] - '0';
	}
	for (int i = 0; i<vm_info_stu->vmMemory.size() - 1; i++) {
		_vmMemory = _vmMemory * 10 + vm_info_stu->vmMemory[i] - '0';
	}
	if (vm_info_stu->vmTwoNodes[0] == '1') {
		_vmTwoNodes = 1;
	}
	else {
		_vmTwoNodes = 0;
	}
	vmInfos[_vmType] = vector<int>{ _vmCpuCores,
		_vmMemory,
		_vmTwoNodes };
}





// 解析用户添加请求
void generateRequestAdd(_generate_requestAdd_info *generate_requestAdd_info)
{
	string _op, _reqVmType, _reqId;

	_op = generate_requestAdd_info->op.substr(1, generate_requestAdd_info->op.size() - 1);  //取出add
	_reqVmType = generate_requestAdd_info->reqVmType.substr(0, generate_requestAdd_info->reqVmType.size() - 1);
	_reqId = generate_requestAdd_info->reqId.substr(0, generate_requestAdd_info->reqId.size() - 1);

	requestInfos.push_back(vector<string>{_op, _reqVmType, _reqId});
}

// 解析用户删除请求
void generateRequestDel(_generate_requestDel_info *generate_requestDel_info)
{
	string _op, _reqId;

	_reqId = generate_requestDel_info->reqId.substr(0, generate_requestDel_info->reqId.size() - 1);
	_op = generate_requestDel_info->op.substr(1, generate_requestDel_info->op.size() - 1);

	requestInfos.push_back(vector<string>{_op, _reqId});
}





// 尝试在服务器上分配虚拟机资源
bool choseServer(vector<int> &server, vector<int> &vm, int serverId, string vmId, int Time)
{
	int vmCores = vm[0],
		vmMemory = vm[1],
		vmTwoNodes = vm[2];

	int &serverCoreA = server[0],
		&serverCoreB = server[1],
		&serverMemoryA = server[2],
		&serverMemoryB = server[3],
		&Node_On_server = server[6];

	if (vmTwoNodes )  
	{
		int needCores = vmCores / 2,
			needMemory = vmMemory / 2;
	 
		if (serverCoreA >= needCores && serverCoreB >= needCores && serverMemoryA >= needMemory && serverMemoryB >= needMemory && ((Time == 1 && (Node_On_server == 2 || Node_On_server == 0)) || Time == 2))
		{
			serverCoreA -= needCores;
			serverCoreB -= needCores;
			serverMemoryA -= needMemory;
			serverMemoryB -= needMemory;
			if (Node_On_server == 0)   Node_On_server = 2;
			NodeOnServerInfo[serverId][0]++;
			vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1,2 };
			res.push_back("(" + to_string(serverId) + ")\n");
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (serverCoreA >= vmCores && serverMemoryA >= vmMemory && ((vmCores >= vmMemory&&serverCoreA >= serverCoreB) || (vmCores< vmMemory&&serverMemoryA >= serverMemoryB)) && ((Time == 1 && (Node_On_server == 1 || Node_On_server == 0)) || Time == 2)) {
			serverCoreA -= vmCores;
			serverMemoryA -= vmMemory;
			if (Node_On_server == 0)   Node_On_server = 1;
            NodeOnServerInfo[serverId][1]++;
			vmOnServer[vmId] = vector<int>{ serverId, vmCores, vmMemory, 1 };
			res.push_back("(" + to_string(serverId) + ", A)\n");
			return true;
	
	}

	else if (serverCoreB >= vmCores && serverMemoryB >= vmMemory && ((Time == 1 && (Node_On_server == 1 || Node_On_server == 0)) || Time == 2)) {

			serverCoreB -= vmCores;
			serverMemoryB -= vmMemory;
			if (Node_On_server == 0)   Node_On_server = 1;
            NodeOnServerInfo[serverId][1]++;
			vmOnServer[vmId] = vector<int>{ serverId, vmCores, vmMemory, 2 };
			res.push_back("(" + to_string(serverId) + ", B)\n");
			return true;
		
	}
	return false;
}


// 处理创建虚拟机操作
int createVM(vector<string> &createVmInfo)
{

	//PredayServerNum, serverNumber;
	string _reqVmType = createVmInfo[1],
		_reqId = createVmInfo[2];
	vector<int> vm = vmInfos[_reqVmType];
	int success = -1;
	int Time = 1;
	for (int i = 0; i<PredayServerNum; i++)
	{
		auto &server = sysServerResource[i];

		/*
		bool choseServer(vector<int> &server,
		vector<int> &vm,
		int serverId,
		string vmId)
		*/
		if (choseServer(server, vm, i, _reqId, Time))
		{
			serverRunVms[i]++;
			success = 1;
			break;
		}
		assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
	}
	//debug
	if (success==-1) {
		Time = 2;
		for (int j = 0; j<PredayServerNum; j++)
		{
			auto &server = sysServerResource[j];

			if (choseServer(server, vm, j, _reqId, Time))
			{
				serverRunVms[j]++;
				success = 1;
				break;
			}
			assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
		}
	}

	if (success == -1) {

		auto &server = sysServerResource[PredayServerNum];
		if (choseServer(server, vm, PredayServerNum, _reqId, Time))
		{
			serverRunVms[PredayServerNum]++;
			success = 1;
			PredayServerNum++;
		}
		assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);

	}

	if (success==-1){
		printf("cnm!");
		system("pause");
	}
	return success;
}

// 处理删除虚拟机操作
void delVM(vector<string> &delVmInfo) {
	string _vmId = delVmInfo[1];
	vector<int> _vmInfo = vmOnServer[_vmId];
	int _serverId = _vmInfo[0];

	serverRunVms[_serverId]--;

	vector<int> &server = sysServerResource[_serverId];
	if (_vmInfo.size() == 5) {
		int cores = _vmInfo[1] / 2, memory = _vmInfo[2] / 2;
		NodeOnServerInfo[_serverId][0]--;
		server[0] += cores;
		server[1] += cores;
		server[2] += memory;
		server[3] += memory;
		if (NodeOnServerInfo[_serverId][0] == 0){
			if (NodeOnServerInfo[_serverId][1] == 0)
				server[6] = 0;
			else
				server[6] = 1;
		}
	}
	else {
		int cores = _vmInfo[1], memory = _vmInfo[2];
		if (_vmInfo[3] == 1) {
			NodeOnServerInfo[_serverId][1]--;
			server[0] += cores;
			server[2] += memory;
			if (NodeOnServerInfo[_serverId][1] == 0){
				if (NodeOnServerInfo[_serverId][0] == 0)
					server[6] = 0;
				else
					server[6] = 2;
			}
		}
		else {
			NodeOnServerInfo[_serverId][1]--;
			server[1] += cores;
			server[3] += memory;
			if (NodeOnServerInfo[_serverId][1] == 0){
				if (NodeOnServerInfo[_serverId][0] == 0)
					server[6] = 0;
				else
					server[6] = 2;
			}
		}
	}
}

//==================================================================================
//寻找虚拟机种类的内核与内存的最大值
//寻找 虚拟机内核和内存的最大值
void   findVm_CM_max(){

	unordered_map<string, vector<int>> ::iterator it;

	for (it = vmInfos.begin(); it != vmInfos.end(); it++) {
		if (it->second[0]> VM_max_Core)
			VM_max_Core = it->second[0];

		if (it->second[1]> VM_max_Mem)
			VM_max_Mem = it->second[1];
	}
}
//==================================================================================
//将服务器的性价比排序：价格/（内核数+内存数)

//哈希key排序,性价比由低到高
bool comp(const pair<int, string>&a, const pair<int, string>&b) {
	return a.first < b.first;
}
vector<pair<int, string>> vec;
void    analyzeServerInfo(){

	for (auto x : serverCPR_info) {
		vec.push_back(x);
	}
	sort(vec.begin(), vec.end(), comp);
}


// 初始化server，如何初始化购买的服务器是一个大的优化
void bugServer() {

	string serverType;
	bool  flag = 0;

	findVm_CM_max();
	analyzeServerInfo();

	for (int i = 0; i < 4000; i++){
		NodeOnServerInfo[i] = vector<int>{0, 0};
		PreNodeOnServerInfo[i] = vector<int>{0, 0};
	}
	
	for (auto it = vec.begin(); it != vec.end(); ++it) {
		if (flag == 0) {
			if (serverInfos[it->second][0] >= VM_max_Core && serverInfos[it->second][2] >= VM_max_Mem)   //服务器内核和内存是最大虚拟需求的2倍，且性价比高
			{
				serverType = it->second;
				flag = 1;
			}
		}
	}

	flag = 0;

	//serverType = "hostUY41I";  hostTUL1P
	//(hostTUL1P, 286, 858, 142387, 176)
	serverType = "hostQ0Y9D";
	int n = 700;  //目前700最佳
	serverRunVms.resize(4000, 0);
	string initBuy = "(purchase, ";
	initBuy += to_string(2) + ")\n";

	//vector<string> res;
	res.push_back(initBuy); //(purchase, 2)

	string pauseInfo = "(" + serverType + ", ";
	pauseInfo += std::to_string(n / 2) + ")\n";

	res.push_back(pauseInfo); //(hostUY41I, 1250)
	day_BuyServers_res.push_back((serverType + std::to_string(0) + "," + std::to_string(n / 2)));
	for (int i = 0; i<n / 2; i++) {
		//unordered_map<int,vector<int>> sysServerResource;
		sysServerResource[serverNumber++] = serverInfos[serverType];
		SERVERCOST += serverInfos[serverType][4];
		Total_Server_ID[serverNumber - 1] = string{ serverType + std::to_string(0)+","+ std::to_string(serverNumber - 1) };
	}
	//Total_Server_NameID[serverType] = int{ n / 2 - 1};
	//ServerTypeBuyOrder[serverType] = int{ 1 };
	//(host78BMY, 996, 332, 246869, 310)
	//(hostUY41I, 676, 994, 243651, 305)
	serverType = "hostC039T";
	pauseInfo = "(" + serverType + ", ";
	pauseInfo += std::to_string(serverNumber) + ")\n";//(host78BMY, 1250)
	
	day_BuyServers_res.push_back( ( serverType + std::to_string(0) + "," + std::to_string(n/2) ));
	res.push_back(pauseInfo);

	for (int i = 0; i<n / 2; i++) {
		sysServerResource[serverNumber++] = serverInfos[serverType];
		SERVERCOST += serverInfos[serverType][4];
		Total_Server_ID[serverNumber - 1] = string{ serverType + std::to_string(0) + "," + std::to_string(serverNumber - 1 - n / 2) };
	}
	//Total_Server_NameID[serverType] = int{ n / 2 - 1 };
	//ServerTypeBuyOrder[serverType] = int{ 2 };
}

// 扩容服务器策略
void expansion() {
	string s = "(purchase, 0)\n";
	res.push_back(s);
}

// 迁移虚拟机策略
void migrate() {
	string s = "(migration, 0)\n";
	res.push_back(s);
}

void serverPower() {
	for (int i = 0; i<serverNumber; i++) {
		if (serverRunVms[i] != 0) {
			POWERCOST += sysServerResource[i][5];
		}
	}
}

//==================================================================================
//分析一天的请求需要部署什么样的虚拟机
//定义全局变量
//每天需要添加的虚拟机类型:  ID + VM信息  不会被覆盖
unordered_map<string, vector<int>>  day_addReq_vmInfos;
void   analyzeRequest(vector<vector<string>> &DayrequestInfos){
	string  _addReq_vmName, _addVmID;
	int   dayVmCore = 0, dayVmMem = 0;
	int _addReqVmCore, _addReqVmMem, _addReqVmNode, VmCore_Div_VmMem;
	//读取当天的请求,只有add操作才有虚拟机名称
	for (auto req : DayrequestInfos) {
		if ( req.size() == 3 ){
			_addVmID = req[2];
			_addReq_vmName = req[1];
			_addReqVmCore = vmInfos[_addReq_vmName][0];
			_addReqVmMem = vmInfos[_addReq_vmName][1];
			_addReqVmNode = vmInfos[_addReq_vmName][2];
			VmCore_Div_VmMem = _addReqVmCore / _addReqVmMem;
			day_addReq_vmInfos[_addVmID] = vector<int>{ _addReqVmCore, _addReqVmMem, _addReqVmNode, VmCore_Div_VmMem };
			dayVmCore += _addReqVmCore;
			dayVmMem += _addReqVmMem;
		}
		//暂时不对del进行处理
	}
}
//==================================================================================
//将每天的请求的add信息的虚拟机资源由大到小进行排序
bool comp_addVm(const pair<string, vector<int>>&a, const pair<string, vector<int>>&b) {
	return a.second[0] + a.second[1] < b.second[0] + b.second[1];
}
//pair向量定义存储 day_addReq_vmInfos的 hash表
vector<pair<string, vector<int>>> addVm_vec;
void   dayVmSort(){

	for (auto x : day_addReq_vmInfos) {
		addVm_vec.push_back(x);
	}
	sort(addVm_vec.begin(), addVm_vec.end(), comp_addVm);
}
//==================================================================================
//重新定义初始化购买策略
void InitServer() {
	//存储该买服务器的名字,并计算相同的服务器购买的台数
	unordered_map<string,int>   BuyServerInfo;
	vector<pair<string, int>> BuyServerInfovec;
	int  Num = 1;
	dayVmSort();
	findVm_CM_max();
	analyzeServerInfo();
	for (auto it = addVm_vec.begin(); it != addVm_vec.end(); ++it) {
		for (auto it_server = vec.begin(); it_server != vec.end(); ++it_server){
			if (4 * it->second[0] <  serverInfos[it_server->second][0] && 4 * it->second[1] <  serverInfos[it_server->second][2] && it->second[3] >= 5 && float(serverInfos[it_server->second][0] / serverInfos[it_server->second][2]) >= 1.5){
				 if (BuyServerInfo.find(it_server->second) != BuyServerInfo.end()){
					 BuyServerInfo[it_server->second] += 1;
				 }
				 else {
					 BuyServerInfo[it_server->second] = int{ Num };
				 }

				sysServerResource[serverNumber++] = serverInfos[it_server->second];
				SERVERCOST += serverInfos[it_server->second][4];
				break;
			}
			else if (4 * it->second[0] <  serverInfos[it_server->second][0] && 4 * it->second[1] <  serverInfos[it_server->second][2] && it->second[3] < 5 && float(serverInfos[it_server->second][0] / serverInfos[it_server->second][2]) < 1.5){
				if (BuyServerInfo.find(it_server->second) != BuyServerInfo.end())
				{
					BuyServerInfo[it_server->second] += 1;
				}
				else{
					BuyServerInfo[it_server->second] = int{ Num };
				}
				sysServerResource[serverNumber++] = serverInfos[it_server->second];
				SERVERCOST += serverInfos[it_server->second][4];

				break;
			}
		
		}
	}

	unordered_map<string, int> ::iterator it;
	//直接给定值，只要不超过购买的服务器总数就OK
	serverRunVms.resize(4000, 0);
	string initBuy = "(purchase, ";
	initBuy += to_string(BuyServerInfo.size()) + ")\n";

	//vector<string> res;
	res.push_back(initBuy); //(purchase, x)

	for (auto x : BuyServerInfo) {
		BuyServerInfovec.push_back(x);
	}
	ofstream out("output1111111.txt");
	for (auto it = BuyServerInfovec.begin(); it != BuyServerInfovec.end(); ++it){
		string pauseInfo = "(" + it->first + ", ";
		pauseInfo += std::to_string(it->second) + ")\n";
		out << pauseInfo << "\n";
		res.push_back(pauseInfo);
	}
}

//==================================================================================
//在扩容之前对当前请求进行一次遍历，再根据该信息来制定扩容策略
bool PrechoseServer(vector<int> &preserver, vector<int> &pre_vm, int serverId, string vmId,int PreTime)
{   
	int vmCores = pre_vm[0],
		vmMemory = pre_vm[1],
		vmTwoNodes = pre_vm[2];

	int &preserverCoreA = preserver[0],
		&preserverCoreB = preserver[1],
		&preserverMemoryA = preserver[2],
		&preserverMemoryB = preserver[3],
		&PreNode_On_server = preserver[6];
	if (vmTwoNodes)
	{
		int needCores = vmCores / 2,
			needMemory = vmMemory / 2;

		if (preserverCoreA >= needCores && preserverCoreB >= needCores && preserverMemoryA >= needMemory && preserverMemoryB >= needMemory && ((PreTime == 1 && PreNode_On_server != 1) || PreTime == 2))
		{
			preserverCoreA -= needCores;
			preserverCoreB -= needCores;
			preserverMemoryA -= needMemory;
			preserverMemoryB -= needMemory;
			if (PreNode_On_server == 0)   PreNode_On_server = 2;
			PreNodeOnServerInfo[serverId][0]++; 
			PrevmOnServer[vmId] = vector<int>{ serverId, vmCores, vmMemory, 1, 2 };
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (preserverCoreA >= vmCores && preserverMemoryA >= vmMemory && ((vmCores >= vmMemory&&preserverCoreA >= preserverCoreB) || (vmCores< vmMemory&&preserverMemoryA >= preserverMemoryB)) && ((PreTime == 1 && (PreNode_On_server == 1 || PreNode_On_server == 0)) || PreTime == 2)) {

		preserverCoreA -= vmCores;
		preserverMemoryA -= vmMemory;
		if (PreNode_On_server == 0)   PreNode_On_server = 1;
		PreNodeOnServerInfo[serverId][1]++;
		PrevmOnServer[vmId] = vector<int>{ serverId, vmCores, vmMemory, 1 };
		return true;

	}

	else if (preserverCoreB >= vmCores && preserverMemoryB >= vmMemory && ((PreTime == 1 && (PreNode_On_server == 1 || PreNode_On_server == 0)) || PreTime == 2)) {

		preserverCoreB -= vmCores;
		preserverMemoryB -= vmMemory;
		if (PreNode_On_server == 0)   PreNode_On_server = 1;
        PreNodeOnServerInfo[serverId][1]++;
		PrevmOnServer[vmId] = vector<int>{ serverId, vmCores, vmMemory, 2 };
		return true;

	}
	return false;
}

//==================================================================================
//在扩容之前对当前请求进行一次遍历，再根据该信息来制定扩容策略: 将每一天的sysServerResource值赋给PresysServerResource，然后再预处理完需求后，进行清除
void     Day_ServersResource(){
	for (int i = 0; i < serverNumber; i++){
		PresysServerResource[i] = sysServerResource[i];
		PreNodeOnServerInfo[i] = NodeOnServerInfo[i];
	}
}

void  Day_PrevmOnServer(){
	unordered_map<string, vector<int>> ::iterator it;
	for (auto it = vmOnServer.begin(); it != vmOnServer.end(); ++it)
		PrevmOnServer[it->first] = vector<int> { it->second };
}
//==================================================================================
//在扩容之前对当前请求进行一次遍历，再根据该信息来制定扩容策略
vector<vector<string>> Exp_requestInfos;
unordered_map<string, int>   EXP_BuyServerInfo;
int  typeNum = 2;  //根初始购买的服务器类型数量有关
int  Total_Buy = 0 ;
int  PrecreateVM(vector<string> &PrecreateVmInfo){
	string _reqVmType = PrecreateVmInfo[1],
		_reqId = PrecreateVmInfo[2];
	vector<int> pre_vm = vmInfos[_reqVmType];    //获取请求的虚拟机的信息
	int success = -1;
	int PreTime = 1;
	int  K = 0;
	for (int i = 0; i < serverNumber; i++)
	{
		auto &preserver = PresysServerResource[i];
		if (PrechoseServer(preserver, pre_vm, i, _reqId, PreTime))
		{
			success = 1;
			break;

		}
		assert(preserver[0] >= 0 && preserver[1] >= 0 && preserver[2] >= 0 && preserver[3] >= 0);
	}
	if (success == -1){
		PreTime = 2;
		//printf("2");
		for (int j = 0; j < serverNumber; j++)
		{
			auto &preserver = PresysServerResource[j];
			if (PrechoseServer(preserver, pre_vm, j, _reqId, PreTime))
			{
				success = 1;
				break;

			}
			assert(preserver[0] >= 0 && preserver[1] >= 0 && preserver[2] >= 0 && preserver[3] >= 0);
		}
	}
	if (success == -1 && PreTime == 2 ) {
		Exp_requestInfos.push_back(vector<string>{"add", _reqVmType, _reqId});

			for (auto it_server = vec.begin(); it_server != vec.end(); ++it_server){

				if (2 * pre_vm[0] <  serverInfos[it_server->second][0] && 2 * pre_vm[1] <  serverInfos[it_server->second][2] && (pre_vm[0] / pre_vm[1]) >= 3 && float(serverInfos[it_server->second][0] / serverInfos[it_server->second][2]) >= 0.8){
					if (EXP_BuyServerInfo.find(it_server->second) != EXP_BuyServerInfo.end()){
						EXP_BuyServerInfo[it_server->second] += 1;
						Total_Server_ID[serverNumber] = string{ it_server->second + std::to_string(dayNum) + "," + std::to_string(EXP_BuyServerInfo[it_server->second]-1) };
					}
					else{
						EXP_BuyServerInfo[it_server->second] = 1;                                                                        //购买数量从1开始
						Total_Server_ID[serverNumber] = string{ it_server->second + std::to_string(dayNum) + "," + std::to_string(0) };  // 编号从0开始
					}
					sysServerResource[serverNumber++] = serverInfos[it_server->second];
					SERVERCOST += serverInfos[it_server->second][4];
				/*	if (Total_Server_NameID.find(it_server->second) != Total_Server_NameID.end()){
						Total_Server_NameID[it_server->second] +=1 ;
						Total_Server_ID[serverNumber - 1] = string{ it_server->second + std::to_string(dayNum) + "," + std::to_string(Total_Server_NameID[it_server->second]) };
					}
					else {
						Total_Server_NameID[it_server->second] = 0;
						ServerTypeBuyOrder[it_server->second] = int{ ++typeNum };
						Total_Server_ID[serverNumber - 1] = string{ it_server->second + std::to_string(0) };
					}*/
					Total_Buy += 1;
					break;
				}
				else if (2 * pre_vm[0] < serverInfos[it_server->second][0] && 2 * pre_vm[1] < serverInfos[it_server->second][2] && (pre_vm[0] / pre_vm[1]) < 3 && float(serverInfos[it_server->second][0] / serverInfos[it_server->second][2]) >= 0.7 && float(serverInfos[it_server->second][0] / serverInfos[it_server->second][2]) <= 1){
					if (EXP_BuyServerInfo.find(it_server->second) != EXP_BuyServerInfo.end()){
						EXP_BuyServerInfo[it_server->second] += 1;
						Total_Server_ID[serverNumber ] = string{ it_server->second + std::to_string(dayNum) + "," + std::to_string(EXP_BuyServerInfo[it_server->second] - 1) };
					}
					else{
						EXP_BuyServerInfo[it_server->second] = 1;                                                                            //购买数量从1开始
						Total_Server_ID[serverNumber ] = string{ it_server->second + std::to_string(dayNum) + "," + std::to_string(0) };  // 编号从0开始
					}

					sysServerResource[serverNumber++] = serverInfos[it_server->second];
					SERVERCOST += serverInfos[it_server->second][4];

	/*				if (Total_Server_NameID.find(it_server->second) != Total_Server_NameID.end()){
						Total_Server_NameID[it_server->second] += 1;
						Total_Server_ID[serverNumber - 1] = string{ it_server->second + std::to_string(Total_Server_NameID[it_server->second]) };
					}
					else {
						Total_Server_NameID[it_server->second] = 0;
						ServerTypeBuyOrder[it_server->second] = int{ ++typeNum };
						Total_Server_ID[serverNumber - 1] = string{ it_server->second + std::to_string(0) };
					}*/
					Total_Buy += 1;

					break;
				}
			}

			PresysServerResource[serverNumber-1] = sysServerResource[serverNumber-1];
			K = PrechoseServer(PresysServerResource[serverNumber - 1], pre_vm, serverNumber - 1, _reqId, PreTime);
			assert(PresysServerResource[serverNumber - 1][0] >= 0 && PresysServerResource[serverNumber - 1][1] >= 0 && PresysServerResource[serverNumber - 1][2] >= 0 && PresysServerResource[serverNumber - 1][3] >= 0);
	}
	return success;
}
//==================================================================================
//预处理处理删除虚拟机操作
void predelVM(vector<string> &predelVmInfo) {
	string _vmId = predelVmInfo[1];
	vector<int> _vmInfo = PrevmOnServer[_vmId];
	int _serverId = _vmInfo[0];

	vector<int> &preserver = PresysServerResource[_serverId];
	if (_vmInfo.size() == 5) {
		int cores = _vmInfo[1] / 2, memory = _vmInfo[2] / 2;
		PreNodeOnServerInfo[_serverId][0]--;
		preserver[0] += cores;
		preserver[1] += cores;
		preserver[2] += memory;
		preserver[3] += memory;
		if (PreNodeOnServerInfo[_serverId][0] == 0){
			if (PreNodeOnServerInfo[_serverId][1] == 0)
				preserver[6] = 0;
			else
				preserver[6] = 1;
		}
	}
	else {
		int cores = _vmInfo[1], memory = _vmInfo[2];
		if (_vmInfo[3] == 1) {
			PreNodeOnServerInfo[_serverId][1]--;
			preserver[0] += cores;
			preserver[2] += memory;
			if (PreNodeOnServerInfo[_serverId][1] == 0){
				if (PreNodeOnServerInfo[_serverId][0] == 0)
					preserver[6] = 0;
				else
					preserver[6] = 2;
			}
		}
		else {
			PreNodeOnServerInfo[_serverId][1]--;
			preserver[1] += cores;
			preserver[3] += memory;
			if (PreNodeOnServerInfo[_serverId][1] == 0){
				if (PreNodeOnServerInfo[_serverId][0] == 0)
					preserver[6] = 0;
				else
					preserver[6] = 2;
			}
		}
	}
}
//==================================================================================
//重新定义扩容策略:先遍历一遍请求，归置一下可以容纳下的请求，不能容纳的请求申请扩容
int buynum = 0;
void expansion_new() {
	bool flag = 0;
	for (auto req : requestInfos) {
		if (req.size() == 3){
			int preresourceEnough = PrecreateVM(req);
			if (preresourceEnough == -1){
				flag = 1;
			}
		}
		else {
				predelVM(req);
		}
	}

	if (flag == 0) {
		string s = "(purchase, 0)\n";
		res.push_back(s);
	}
	else {

		//analyzeRequest(Exp_requestInfos);
		//存储该买服务器的名字,并计算相同的服务器购买的台数
		//unordered_map<string, int>   EXP_BuyServerInfo;
		vector<pair<string, int>> EXP_BuyServerInfovec;
		//int  Num = 1;
		//dayVmSort();

		unordered_map<string, int>   EXP_BuyServerInfo_reg;
		unordered_map<string, vector<int>> ::iterator it_reg;
		for (auto it_reg = EXP_BuyServerInfo.begin(); it_reg != EXP_BuyServerInfo.end(); ++it_reg){
			EXP_BuyServerInfo_reg[it_reg->first] = int{ it_reg->second };
		}
		EXP_BuyServerInfo.clear();
		unordered_map<string, int> ::iterator it;
		string initBuy = "(purchase, ";
		initBuy += to_string(EXP_BuyServerInfo_reg.size()) + ")\n";
		//vector<string> res;
		res.push_back(initBuy); //(purchase, x)
		for (auto x : EXP_BuyServerInfo_reg) {
			EXP_BuyServerInfovec.push_back(x);
		}

		for (auto it = EXP_BuyServerInfovec.begin(); it != EXP_BuyServerInfovec.end(); ++it){
			string pauseInfo = "(" + it->first + ", ";
			pauseInfo += std::to_string(it->second) + ")\n";
			buynum += it->second;
			day_BuyServers_res.push_back((it->first + std::to_string(dayNum) + "," + std::to_string(it->second)));
			res.push_back(pauseInfo);
		}
	
	}
}

//======================================================================================
//分配策略目前 day=requestdays
void match(int day) {
	if (day != 0)
	{
		PredayServerNum = serverNumber;
		Day_ServersResource();
		Day_PrevmOnServer();
		expansion_new();
	}

	migrate();
	//    printf("There are %d requests waiting to matching !!\n",requestInfos.size());
	for (auto req : requestInfos) {
		// 创建虚拟机 还是 删除虚拟机
		int opType = req.size() == 3 ? 1 : 0;
		if (opType) {
			int resourceEnough = createVM(req);
			assert(resourceEnough != -1);
		}
		else
		{
			// 修复删除虚拟机bug
			delVM(req);
		}
	}
}

//bool Servercompup(const pair<string, int>&a, const pair<string, int>&b){
//	return a.second < b.second;
//}
//判断字符串是否是数字
bool isNum(string str)
{
	stringstream sin(str);
	double d;
	char c;
	if (!(sin >> d))
		return false;
	if (sin >> c)
		return false;
	return true;
}

int main() {
	clock_t start, finish;
	start = clock();

	//@@@@@@@@@@@@@@@@@@@@ struct definition @@@@@@@@@@@@@@@@@@@@@@
	_server_info_stu server_info_stu;//store   server information
	_vm_info_stu vm_info_stu;      //store  vm information
	_generate_requestAdd_info generate_requestAdd_info;
	_generate_requestDel_info generate_requestDel_info;
	//@@@@@@@@@@@@@@@@@@@@  end @@@@@@@@@@@@@@@@@@@@@@



		//在读取文件时，采用freopen进行重定向到txt文件，采用cin标准输入读取数据
#ifdef TEST
	    FILE *s1;
		freopen_s(&s1,filePath.c_str(), "rb", stdin);
#endif
	int serverNum;

	scanf("%d", &serverNum);

	for (int i = 0; i<serverNum; i++)
	{
		cin >> server_info_stu.serverType >> server_info_stu.cpuCores >> server_info_stu.memorySize >> server_info_stu.serverCost >> server_info_stu.powerCost;

		generateServer(&server_info_stu);
	}


	int vmNumber = 0;
	scanf("%d", &vmNumber);



	for (int i = 0; i<vmNumber; i++) {
		cin >> vm_info_stu.vmType >> vm_info_stu.vmCpuCores >> vm_info_stu.vmMemory >> vm_info_stu.vmTwoNodes;

		generateVm(&vm_info_stu);

	}

	int requestdays = 0,
		dayRequestNumber = 0;

	scanf("%d", &requestdays);




	// 开始处理请求
	//bugServer();
	for (int day = 0; day<requestdays; day++)
	{
		scanf("%d", &dayRequestNumber);
		PresysServerResource.clear();
		PrevmOnServer.clear();
		addVm_vec.clear();
		day_addReq_vmInfos.clear();
		requestInfos.clear();
		
		for (int i = 0; i<dayRequestNumber; i++) {
			cin >> generate_requestAdd_info.op;
			if (generate_requestAdd_info.op[1] == 'a') {  //应该是对的   op应该是为(add,xx,xx)
				cin >> generate_requestAdd_info.reqVmType >> generate_requestAdd_info.reqId;
			   //generateRequestAdd(op,reqVmType,reqId);
				generateRequestAdd(&generate_requestAdd_info);
			}
			else {
				generate_requestDel_info.op = generate_requestAdd_info.op;
				cin >> generate_requestDel_info.reqId;
				//generateRequestDel(op,reqId);
				generateRequestDel(&generate_requestDel_info);
			}
		}
#ifdef TEST
		if (day == 0 || (day + 1) % 100 == 0) {
			printf("The %d day begin matching!!!\n", day + 1);
		}
#endif
		if (day == 0) {
			//analyzeRequest(requestInfos);
			bugServer();
			PredayServerNum = serverNumber;
		}
		dayNum++;
		match(day);  //目前 day=requestdays
		serverPower();
		//        break;
	}

	fclose(stdin);
	finish = clock();
	TOTALCOST = SERVERCOST + POWERCOST;
#ifdef UPLOAD

	vector<string>   res_New;
	string  ID_new_str;
	int old_ID;
	int New_ID;
	string ServerName_Day;
	int ServerDayType_num;
	string ServerName;
	int   Temp = 0;
	//unordered_map<int, string> ::iterator it_IDYS;
	//for (auto it_IDYS = Total_Server_ID.begin(); it_IDYS != Total_Server_ID.end(); ++it_IDYS)
	//ServerTypeBuyOrder  Total_Server_NameID
	unordered_map<string, int>    REAL_NEWID_Start;
	//unordered_map<string,int> ::iterator it_A;

	//哈希key排序,由小到大

	//vector<pair<string, int>> ServerTypeBuyOrdervec;
	//for (auto x : ServerTypeBuyOrder) { ServerTypeBuyOrdervec.push_back(x); }

	//sort(ServerTypeBuyOrdervec.begin(), ServerTypeBuyOrdervec.end(), Servercompup);

	//解析 day_BuyServers_res 的信息
	for (auto &dayBuyS: day_BuyServers_res){

		ServerName_Day = dayBuyS.substr(0, dayBuyS.find(','));
		REAL_NEWID_Start[ServerName_Day] = int{ Temp };
		ServerDayType_num = std::stoi(dayBuyS.substr(dayBuyS.find(',') + 1));
		Temp += ServerDayType_num;
	}

	for (auto &s : res){
		if (s.find(',') == string::npos){
			old_ID = std::stoi(s.substr(1, s.find(')') - 1));
			ServerName = Total_Server_ID[old_ID].substr(0, Total_Server_ID[old_ID].find(',') );
			New_ID = std::stoi(Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1)) + REAL_NEWID_Start[ServerName];
			ID_new_str = "(" + std::to_string(New_ID)+ ")\n";
			res_New.push_back(ID_new_str);
		}
		else {
			if (isNum(s.substr(1, 2)) == 1){
				old_ID = std::stoi(s.substr(1, s.find(',') - 1));
				ServerName = Total_Server_ID[old_ID].substr(0, Total_Server_ID[old_ID].find(','));
				New_ID = std::stoi(Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1)) + REAL_NEWID_Start[ServerName];
				ID_new_str = "(" + std::to_string(New_ID) + s.substr(s.find(','));
				res_New.push_back(ID_new_str);
			}
			else {
				res_New.push_back(s);
			}
		}
	}
	for (auto &s : res_New) std::cout << s;
#endif
#ifdef TEST
	printf("\nusr time: %f s \n", double(finish - start) / CLOCKS_PER_SEC);
	printf("server cost: %lld \npower cost: %lld \ntotal cost: %lld \n", SERVERCOST, POWERCOST, TOTALCOST);
	ofstream out("output.txt");
	 out << "server cost: " << SERVERCOST << "\n";
	 out << "power cost:  " << POWERCOST  << "\n";
	 out << "total cost : " << TOTALCOST  << "\n";

	 vector<string>   res_New;
	 string  ID_new_str;
	 int old_ID;
	 int New_ID;
	 string ServerName_Day;
	 int ServerDayType_num;
	 string ServerName;
	 int   Temp = 0;
	 //unordered_map<int, string> ::iterator it_IDYS;
	 //for (auto it_IDYS = Total_Server_ID.begin(); it_IDYS != Total_Server_ID.end(); ++it_IDYS)
	 //ServerTypeBuyOrder  Total_Server_NameID
	  unordered_map<string, int>    REAL_NEWID_Start;
	 //unordered_map<string,int> ::iterator it_A;

//哈希key排序,由小到大

	 //vector<pair<string, int>> ServerTypeBuyOrdervec;
	 //for (auto x : ServerTypeBuyOrder) { ServerTypeBuyOrdervec.push_back(x); }

	 //sort(ServerTypeBuyOrdervec.begin(), ServerTypeBuyOrdervec.end(), Servercompup);

//解析 day_BuyServers_res 的信息
	 for (auto &dayBuyS: day_BuyServers_res){

		 ServerName_Day = dayBuyS.substr(0, dayBuyS.find(','));
		 REAL_NEWID_Start[ServerName_Day] = int{ Temp };
		 ServerDayType_num = std::stoi(dayBuyS.substr(dayBuyS.find(',') + 1));
		 //out << dayBuyS<<"---->"<<ServerName_Day << ":" << Temp << "\n";
		 Temp += ServerDayType_num;
	 }

	 for (auto &s : res){
		 if (s.find(',') == string::npos){
			 old_ID = std::stoi(s.substr(1, s.find(')') - 1));
			 ServerName = Total_Server_ID[old_ID].substr(0, Total_Server_ID[old_ID].find(',') );
			 New_ID = std::stoi(Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1)) + REAL_NEWID_Start[ServerName];
			/* out << ServerName << ":" << New_ID << ":" << old_ID << ":" << REAL_NEWID_Start[ServerName] << ":" << Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1) << "\n";*/
			 ID_new_str = "(" + std::to_string(New_ID)+ ")\n";
			 res_New.push_back(ID_new_str);
		 }
		 else {
			 if (isNum(s.substr(1, 2)) == 1){
				 old_ID = std::stoi(s.substr(1, s.find(',') - 1));
				 ServerName = Total_Server_ID[old_ID].substr(0, Total_Server_ID[old_ID].find(','));
				 New_ID = std::stoi(Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1)) + REAL_NEWID_Start[ServerName];
				 //out << ServerName << ":" << New_ID << ":" << old_ID << ":" << REAL_NEWID_Start[ServerName] << ":" << Total_Server_ID[old_ID].substr(Total_Server_ID[old_ID].find(',') + 1) << "\n";
				 ID_new_str = "(" + std::to_string(New_ID) + s.substr(s.find(','));
				 res_New.push_back(ID_new_str);
			 }
			 else {
				 res_New.push_back(s);
			 }
		 }
	 }
	 out << "::::" << Total_Buy << "::" << serverNumber << "\n";
	 for (auto &s : res_New) out << s;
	//for (auto &s : res) std::cout << s;
#endif

	return 0;
}
