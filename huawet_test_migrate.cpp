#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cassert>
using namespace std;

// 提交
//#define UPLOAD

#define TEST

//#define My_PRINT

int flag_AorB = 0;  //其中 flag_AorB = 2表示虚拟机搭建在服务器A节点上   = 3 表示虚拟机构架在服务器B节点上

					// 服务器信息
unordered_map<string, vector<int>> serverInfos;
// 虚拟机信息
unordered_map<string, vector<int>> vmInfos;
// 请求信息
vector<vector<string>> requestInfos;

// 购买的服务器信息
int serverNumber = 0;
unordered_map<int, vector<int>> sysServerResource;

// 当前开机服务器
vector<int> serverRunVms;
// 记录虚拟机运行在那个服务器上
unordered_map<string, vector<int>> vmOnServer;

/*
记录每天添加/删减的虚拟机CPU及内存数量及单双核数量-分单双节点部署(用结构体)
*/
struct _per_day_VmTotal_info {

	int deployed_Vm_number = 0;

	int Per_Day_VmTotal_CPU = 0;
	int Per_Day_VmTotal_MEM = 0;

	int Per_Day_VmTotal_DoubleNodeCPU = 0;
	int Per_Day_VmTotal_DoubleNodeMEM = 0;


	int Per_Day_VmTotal_SingeNode = 0;
	int Per_Day_VmTotal_DoubleNode = 0;

}per_day_VmTotal_info;

/*
记录每天购买的服务器类型，包括
购买服务器的类型
服务器ID(编号从0开始，每增加一台编号增加1
服务器用途(目前用于为虚拟机单双节点部署flag，0表示此编号服务器用于单节点部署
1表示此编号服务器用于双节点部署
服务器A节点的剩余CPU核数(初始为total cpu/2)
服务器B节点的剩余CPU核数
服务器A节点的剩余MEM(初始为total mem/2)
服务器B节点的剩余MEM

还需要记录每个服务器上所负载的虚拟机的ID，为了migration
*/

int ServerNumberALL = 2600;


struct _purchase_service_info {
	string purchase_service_type;

	int purchased_Service_number;

	/*除了记录服务器ID（注意服务器ID对应的信息可以从其他参数导出）
	还需要记录每个服务器上所负载的虚拟机的ID，为了migration*/
	unordered_map<int, vector<int>> purchase_service_ID_Info;
	/*
	purchase_service_ID_Info[i][j]  :s
	其中i表示 购买的服务器序号，目前是和服务器ID一致
	其中j = 0时，存储服务器ID ; j = 1 2 3 时，表示搭建的虚拟机编号
	*/

	//vector<int> purchase_service_ID;
	vector<int> purchase_service_useflag;
	vector<int> purchase_service_nodeA_remainCPU;
	vector<int> purchase_service_nodeB_remainCPU;
	vector<int> purchase_service_nodeA_remainMEM;
	vector<int> purchase_service_nodeB_remainMEM;

}purchase_service_info;


/*
迁移虚拟机思路
首先明白迁移的意义：(1)避免出现极端情况（即存在CPU占有量或者MEM占有量单独很大
,而与之对应的MEM占有量或者CPU量很小）;
(2)因为删除虚拟机的操作，导致之前的服务器上CPU/MEM可能有比较多的剩余
所以重新将那些服务器利用率低的虚拟机进行迁移，降低功耗成本;
处理过程需要注意每天虚拟机迁移的数量



*/



vector<string> res;

#ifdef TEST
const string filePath = "training-1.txt";
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







/*@@@@@@@@@@@@@@@@@@@@@@@@@@@ add  struct here @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
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

struct _userVm_requestAdd_info {
	string op;
	string reqVmType;
	string reqId;
};

struct _userVm_requestDel_info {
	string op;
	string reqId;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@ end @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/







void generateServer(_server_info_stu *server_info_stu)
{
	string _serverType = "";
	for (int i = 1; i < server_info_stu->serverType.size() - 1; i++) {
		_serverType += server_info_stu->serverType[i];
	}

	int _cpuCores = 0,
		_memorySize = 0,
		_serverCost = 0,
		_powerCost = 0;

	for (int i = 0; i < server_info_stu->cpuCores.size() - 1; i++)
	{
		_cpuCores = 10 * _cpuCores + server_info_stu->cpuCores[i] - '0';
	}

	for (int i = 0; i < server_info_stu->memorySize.size() - 1; i++)
	{
		_memorySize = 10 * _memorySize + server_info_stu->memorySize[i] - '0';
	}

	for (int i = 0; i < server_info_stu->serverCost.size() - 1; i++)
	{
		_serverCost = 10 * _serverCost + server_info_stu->serverCost[i] - '0';
	}

	for (int i = 0; i < server_info_stu->powerCost.size() - 1; i++)
	{
		_powerCost = 10 * _powerCost + server_info_stu->powerCost[i] - '0';
	}

	serverInfos[_serverType] = vector<int>{ _cpuCores / 2 ,
		_cpuCores / 2,
		_memorySize / 2,
		_memorySize / 2,
		_serverCost,
		_powerCost };
}

/*  解析txt文件时，将可售卖虚拟机类型信息解析保存
(型号，cpu核数，内存大小，是否双节点部署)
*/
void generateVm(_vm_info_stu *vm_info_stu)
{
	string _vmType;

	for (int i = 1; i < vm_info_stu->vmType.size() - 1; i++) {
		_vmType += vm_info_stu->vmType[i];
	}

	int _vmCpuCores = 0, _vmMemory = 0, _vmTwoNodes = 0;
	for (int i = 0; i < vm_info_stu->vmCpuCores.size() - 1; i++) {
		_vmCpuCores = _vmCpuCores * 10 + vm_info_stu->vmCpuCores[i] - '0';
	}
	for (int i = 0; i < vm_info_stu->vmMemory.size() - 1; i++) {
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
void userVmRequestAdd(_userVm_requestAdd_info *userVm_requestAdd_info)
{
	string _op, _reqVmType, _reqId;

	_op = userVm_requestAdd_info->op.substr(1, userVm_requestAdd_info->op.size() - 1);  //取出add
	_reqVmType = userVm_requestAdd_info->reqVmType.substr(0, userVm_requestAdd_info->reqVmType.size() - 1);
	_reqId = userVm_requestAdd_info->reqId.substr(0, userVm_requestAdd_info->reqId.size() - 1);

	requestInfos.push_back(vector<string>{_op, _reqVmType, _reqId});
}

// 解析用户删除请求
void userVmRequestDel(_userVm_requestDel_info *userVm_requestDel_info)
{
	string _op, _reqId;

	_reqId = userVm_requestDel_info->reqId.substr(0, userVm_requestDel_info->reqId.size() - 1);
	_op = userVm_requestDel_info->op.substr(1, userVm_requestDel_info->op.size() - 1);

	requestInfos.push_back(vector<string>{_op, _reqId});
}





// 尝试在服务器上分配虚拟机资源
bool choseServer(vector<int> &server,
	vector<int> &vm,
	int serverId,
	string vmId)
{
	int vmCores = vm[0],
		vmMemory = vm[1],
		vmTwoNodes = vm[2];

	int &serverCoreA = server[0],
		&serverCoreB = server[1],
		&serverMemoryA = server[2],
		&serverMemoryB = server[3];

	if (vmTwoNodes)
	{
		int needCores = vmCores / 2,
			needMemory = vmMemory / 2;

		if (serverCoreA >= needCores && serverCoreB >= needCores && serverMemoryA >= needMemory && serverMemoryB >= needMemory)
		{
			serverCoreA -= needCores;
			serverCoreB -= needCores;
			serverMemoryA -= needMemory;
			serverMemoryB -= needMemory;
			vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1,2 };
			res.push_back("(" + to_string(serverId) + ")\n");
#ifdef My_PRINT
			cout << "(" + to_string(serverId) + ")\n" << endl;
#endif
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (serverCoreA >= vmCores && serverMemoryA >= vmMemory) {

		flag_AorB = 2; //部署在A节点上

		serverCoreA -= vmCores;
		serverMemoryA -= vmMemory;
		vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1 };
		res.push_back("(" + to_string(serverId) + ", A)\n");
#ifdef My_PRINT
		cout << "(" + to_string(serverId) + ", A)\n" << endl;
#endif
		return true;
	}
	else if (serverCoreB >= vmCores && serverMemoryB >= vmMemory) {

		flag_AorB = 3; //部署在B节点上

		serverCoreB -= vmCores;
		serverMemoryB -= vmMemory;
		vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,2 };
		res.push_back("(" + to_string(serverId) + ", B)\n");
#ifdef My_PRINT
		cout << "(" + to_string(serverId) + ", B)\n" << endl;
#endif
		return true;
	}
	return false;
}


// 处理创建虚拟机操作

//createVmInfo<----->requestInfos =  {_op, _reqVmType, _reqId}); or {_op, _reqId});
int createVM(vector<string> &createVmInfo)
{
	string _reqVmType = createVmInfo[1],
		_reqId = createVmInfo[2];

	/*
	vmInfos[_vmType] = vector<int>{ _vmCpuCores,
	_vmMemory,
	_vmTwoNodes };
	*/



	/*
	转换虚拟机的ID号，string 转 int
	转换虚拟机的CPU及MEM  sting 转 int  --->可以不用
	*/
	//目前虚拟机都可以安装得下，所以直接++
	per_day_VmTotal_info.deployed_Vm_number++;


	int vm_tras_ID = 0;
	for (int i_trans = 0; i_trans < _reqId.size(); i_trans++)
	{
		vm_tras_ID = 10 * vm_tras_ID + _reqId[i_trans] - '0';
	}


	//int  temp_i = 0;
	int  temp_flag = 0;

	vector<int> vm = vmInfos[_reqVmType];
	int success = -1;


	/*
	int vmCores = vm[0],
	vmMemory = vm[1],
	*/
	//先走分类循环，如果0 - serverNumber/2已经满了，转移到serverNumber/2 - serverNumber
	if ((vm[0] / vm[1]) <= 1)
	{
		for (int i = 0; i < serverNumber / 2; i++)
		{
			auto &server = sysServerResource[i];

			/*
			更新服务器剩余的信息
			*/
			server[0] = purchase_service_info.purchase_service_nodeA_remainCPU[i]; //A节点剩余CPU
			server[1] = purchase_service_info.purchase_service_nodeB_remainCPU[i]; //B节点剩余CPU
			server[2] = purchase_service_info.purchase_service_nodeA_remainMEM[i]; //A节点剩余MEM
			server[3] = purchase_service_info.purchase_service_nodeB_remainMEM[i]; //B节点剩余MEM



																				   /*
																				   bool choseServer(vector<int> &server,
																				   vector<int> &vm,


																				   int serverId,
																				   string vmId)
																				   */

																				   /*迁移后修改服务器剩下的mem*/


#ifdef My_PRINT
			if (vm[2] == 0)
			{
				cout << "single nodeB" << "remainCPU  " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
			}
#endif
			if (choseServer(server, vm, i, _reqId))
			{
				serverRunVms[i]++;
				success = 1;

				temp_flag = 1;


				/*添加虚拟机负载的信息  默认，load_vm <= 29*/
				for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
				{
					if (purchase_service_info.purchase_service_ID_Info[i][3 + 4 * load_vm] == 0)
					{
						purchase_service_info.purchase_service_ID_Info[i][3 + 4 * load_vm] = vm_tras_ID;

						if (vm[2] == 0)  //节点为0时，赋值为2
						{
							if (flag_AorB == 2)
							{
								purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = 2;

								flag_AorB = 0;
							}
							else if (flag_AorB == 3)
							{
								purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = 3;

								flag_AorB = 0;
							}
						}
						else
						{
							purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = vm[2];   //Nodes
						}

						purchase_service_info.purchase_service_ID_Info[i][5 + 4 * load_vm] = vm[0];   //CPU
						purchase_service_info.purchase_service_ID_Info[i][6 + 4 * load_vm] = vm[1];   //MEM

						if (vm_tras_ID == 20835050)
						{
							cout << "serverID:" << i << "   @vmID:" << vm_tras_ID << "   @4 + 4 * load_vm: " << (4 + 4 * load_vm) << endl;
							cout << "00000000:@" << load_vm << endl;
						}

						break;
					}

					if (load_vm > 73)
					{
						for (int my_iii = 0; my_iii < 74; my_iii++)
						{
							cout << "debug :" << i << "   @purchase_service_ID_Info: " << purchase_service_info.purchase_service_ID_Info[i][3 + 4 * my_iii] << endl;
						}
					}

					assert(load_vm < 74);
				}

				//重新更新服务器剩余的信息
				purchase_service_info.purchase_service_nodeA_remainCPU[i] = server[0]; //A节点剩余CPU
				purchase_service_info.purchase_service_nodeB_remainCPU[i] = server[1]; //B节点剩余CPU
				purchase_service_info.purchase_service_nodeA_remainMEM[i] = server[2]; //A节点剩余MEM
				purchase_service_info.purchase_service_nodeB_remainMEM[i] = server[3]; //B节点剩余MEM

																					   /*   @@@@@@@@@@@@  end of it @@@@@@@@@@@@@@@*/

#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU-del " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
				}
#endif

				break;
			}

			//重新更新服务器剩余的信息
			purchase_service_info.purchase_service_nodeA_remainCPU[i] = server[0]; //A节点剩余CPU
			purchase_service_info.purchase_service_nodeB_remainCPU[i] = server[1]; //B节点剩余CPU
			purchase_service_info.purchase_service_nodeA_remainMEM[i] = server[2]; //A节点剩余MEM
			purchase_service_info.purchase_service_nodeB_remainMEM[i] = server[3]; //B节点剩余MEM

#ifdef My_PRINT
			if (vm[2] == 0)
			{
				cout << "single nodeB" << "remainCPU-kep " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
			}
#endif

			assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
			//temp_i = i;




			temp_flag = 0;
		}

		if (temp_flag == 0)
		{
			for (int temp_i = serverNumber / 2; temp_i < serverNumber; temp_i++)
			{
				auto &server = sysServerResource[temp_i];

				/*
				更新服务器剩余的信息
				*/
				server[0] = purchase_service_info.purchase_service_nodeA_remainCPU[temp_i]; //A节点剩余CPU
				server[1] = purchase_service_info.purchase_service_nodeB_remainCPU[temp_i]; //B节点剩余CPU
				server[2] = purchase_service_info.purchase_service_nodeA_remainMEM[temp_i]; //A节点剩余MEM
				server[3] = purchase_service_info.purchase_service_nodeB_remainMEM[temp_i]; //B节点剩余MEM


#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
				}
#endif

				/*
				bool choseServer(vector<int> &server,
				vector<int> &vm,
				int serverId,
				string vmId)
				*/
				if (choseServer(server, vm, temp_i, _reqId))
				{
					serverRunVms[temp_i]++;
					success = 1;


					/*添加虚拟机负载的信息  默认，load_vm <= 29*/
					for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
					{
						if (purchase_service_info.purchase_service_ID_Info[temp_i][3 + 4 * load_vm] == 0)
						{
							purchase_service_info.purchase_service_ID_Info[temp_i][3 + 4 * load_vm] = vm_tras_ID;

							if (vm[2] == 0)  //节点为0时，赋值为2
							{
								if (flag_AorB == 2)
								{
									purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = 2;

									flag_AorB = 0;
								}
								else if (flag_AorB == 3)
								{
									purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = 3;

									flag_AorB = 0;
								}
							}
							else
							{
								purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = vm[2];   //Nodes
							}

							purchase_service_info.purchase_service_ID_Info[temp_i][5 + 4 * load_vm] = vm[0];   //CPU
							purchase_service_info.purchase_service_ID_Info[temp_i][6 + 4 * load_vm] = vm[1];   //MEM


							if (vm_tras_ID == 20835050)
							{
								cout << "serverID:" << temp_i << "   @vmID:" << vm_tras_ID << "   @4 + 4 * load_vm: " << (4 + 4 * load_vm) << endl;
							}


							break;
						}

						assert(load_vm < 74);
					}

					//重新更新服务器剩余的信息
					purchase_service_info.purchase_service_nodeA_remainCPU[temp_i] = server[0]; //A节点剩余CPU
					purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] = server[1]; //B节点剩余CPU
					purchase_service_info.purchase_service_nodeA_remainMEM[temp_i] = server[2]; //A节点剩余MEM
					purchase_service_info.purchase_service_nodeB_remainMEM[temp_i] = server[3]; //B节点剩余MEM


#ifdef My_PRINT
					if (vm[2] == 0)
					{
						cout << "single nodeB" << "remainCPU-del " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
					}
#endif
					break;
				}

				//重新更新服务器剩余的信息
				purchase_service_info.purchase_service_nodeA_remainCPU[temp_i] = server[0]; //A节点剩余CPU
				purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] = server[1]; //B节点剩余CPU
				purchase_service_info.purchase_service_nodeA_remainMEM[temp_i] = server[2]; //A节点剩余MEM
				purchase_service_info.purchase_service_nodeB_remainMEM[temp_i] = server[3]; //B节点剩余MEM

#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU-kep " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
				}
#endif

				assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
			}
		}

	}
	/*先走分类循环，如果serverNumber/2 - serverNumber已经满了，转移到0 -serverNumber/2
	if ((vm[0] / vm[1]) > 1)*/
	else
	{
		for (int i = serverNumber / 2; i < serverNumber; i++)
		{
			auto &server = sysServerResource[i];

			/*
			bool choseServer(vector<int> &server,
			vector<int> &vm,
			int serverId,
			string vmId)
			*/
			/*
			更新服务器剩余的信息
			*/
			server[0] = purchase_service_info.purchase_service_nodeA_remainCPU[i]; //A节点剩余CPU
			server[1] = purchase_service_info.purchase_service_nodeB_remainCPU[i]; //B节点剩余CPU
			server[2] = purchase_service_info.purchase_service_nodeA_remainMEM[i]; //A节点剩余MEM
			server[3] = purchase_service_info.purchase_service_nodeB_remainMEM[i]; //B节点剩余MEM

#ifdef My_PRINT
			if (vm[2] == 0)
			{
				cout << "single nodeB" << "remainCPU " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
			}
#endif


			if (choseServer(server, vm, i, _reqId))
			{
				serverRunVms[i]++;
				success = 1;

				temp_flag = 1;

				/*添加虚拟机负载的信息  默认，load_vm <= 29*/
				for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
				{
					if (purchase_service_info.purchase_service_ID_Info[i][3 + 4 * load_vm] == 0)
					{
						purchase_service_info.purchase_service_ID_Info[i][3 + 4 * load_vm] = vm_tras_ID;

						if (vm[2] == 0)  //节点为0时，赋值为2
						{
							if (flag_AorB == 2)
							{
								purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = 2;

								flag_AorB = 0;
							}
							else if (flag_AorB == 3)
							{
								purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = 3;

								flag_AorB = 0;
							}
						}
						else
						{
							purchase_service_info.purchase_service_ID_Info[i][4 + 4 * load_vm] = vm[2];   //Nodes
						}

						purchase_service_info.purchase_service_ID_Info[i][5 + 4 * load_vm] = vm[0];   //CPU
						purchase_service_info.purchase_service_ID_Info[i][6 + 4 * load_vm] = vm[1];   //MEM

						if (vm_tras_ID == 20835050)
						{
							cout << "serverID:" << i << "   @vmID:" << vm_tras_ID << "   @4 + 4 * load_vm: " << (4 + 4 * load_vm) << endl;
						}

						break;
					}

					assert(load_vm < 74);
				}


				//重新更新服务器剩余的信息
				purchase_service_info.purchase_service_nodeA_remainCPU[i] = server[0]; //A节点剩余CPU
				purchase_service_info.purchase_service_nodeB_remainCPU[i] = server[1]; //B节点剩余CPU
				purchase_service_info.purchase_service_nodeA_remainMEM[i] = server[2]; //A节点剩余MEM
				purchase_service_info.purchase_service_nodeB_remainMEM[i] = server[3]; //B节点剩余MEM


#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU-del " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
				}
#endif

				break;
			}

			//重新更新服务器剩余的信息
			purchase_service_info.purchase_service_nodeA_remainCPU[i] = server[0]; //A节点剩余CPU
			purchase_service_info.purchase_service_nodeB_remainCPU[i] = server[1]; //B节点剩余CPU
			purchase_service_info.purchase_service_nodeA_remainMEM[i] = server[2]; //A节点剩余MEM
			purchase_service_info.purchase_service_nodeB_remainMEM[i] = server[3]; //B节点剩余MEM

#ifdef My_PRINT
			if (vm[2] == 0)
			{
				cout << "single nodeB" << "remainCPU-kep " << purchase_service_info.purchase_service_nodeB_remainCPU[i] << endl;
			}
#endif

			assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
			temp_flag = 0;
		}
		//temp_i = 0;
		if (temp_flag == 0)
		{
			for (int temp_i = 0; temp_i < serverNumber / 2; temp_i++)
			{
				auto &server = sysServerResource[temp_i];

				/*
				更新服务器剩余的信息
				*/
				server[0] = purchase_service_info.purchase_service_nodeA_remainCPU[temp_i]; //A节点剩余CPU
				server[1] = purchase_service_info.purchase_service_nodeB_remainCPU[temp_i]; //B节点剩余CPU
				server[2] = purchase_service_info.purchase_service_nodeA_remainMEM[temp_i]; //A节点剩余MEM
				server[3] = purchase_service_info.purchase_service_nodeB_remainMEM[temp_i]; //B节点剩余MEM


#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
				}
#endif

				/*
				bool choseServer(vector<int> &server,
				vector<int> &vm,
				int serverId,
				string vmId)
				*/
				if (choseServer(server, vm, temp_i, _reqId))
				{
					serverRunVms[temp_i]++;
					success = 1;

					/*添加虚拟机负载的信息  默认，load_vm <= 29   39*/
					for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
					{
						if (purchase_service_info.purchase_service_ID_Info[temp_i][3 + 4 * load_vm] == 0)
						{
							purchase_service_info.purchase_service_ID_Info[temp_i][3 + 4 * load_vm] = vm_tras_ID;
							if (vm[2] == 0)  //节点为0时，赋值为2
							{
								if (flag_AorB == 2)
								{
									purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = 2;

									flag_AorB = 0;
								}
								else if (flag_AorB == 3)
								{
									purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = 3;

									flag_AorB = 0;
								}
							}
							else
							{
								purchase_service_info.purchase_service_ID_Info[temp_i][4 + 4 * load_vm] = vm[2];   //Nodes
							}
							purchase_service_info.purchase_service_ID_Info[temp_i][5 + 4 * load_vm] = vm[0];   //CPU
							purchase_service_info.purchase_service_ID_Info[temp_i][6 + 4 * load_vm] = vm[1];   //MEM

							if (vm_tras_ID == 20835050)
							{
								cout << "serverID:" << temp_i << "   @vmID:" << vm_tras_ID << "   @4 + 4 * load_vm: " << (4 + 4 * load_vm) << endl;
							}


							break;
						}

						assert(load_vm < 74);
					}

					//重新更新服务器剩余的信息
					purchase_service_info.purchase_service_nodeA_remainCPU[temp_i] = server[0]; //A节点剩余CPU
					purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] = server[1]; //B节点剩余CPU
					purchase_service_info.purchase_service_nodeA_remainMEM[temp_i] = server[2]; //A节点剩余MEM
					purchase_service_info.purchase_service_nodeB_remainMEM[temp_i] = server[3]; //B节点剩余MEM

#ifdef My_PRINT
					if (vm[2] == 0)
					{
						cout << "single nodeB" << "remainCPU-del " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
					}
#endif


					break;
				}

				//重新更新服务器剩余的信息
				purchase_service_info.purchase_service_nodeA_remainCPU[temp_i] = server[0]; //A节点剩余CPU
				purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] = server[1]; //B节点剩余CPU
				purchase_service_info.purchase_service_nodeA_remainMEM[temp_i] = server[2]; //A节点剩余MEM
				purchase_service_info.purchase_service_nodeB_remainMEM[temp_i] = server[3]; //B节点剩余MEM

#ifdef My_PRINT
				if (vm[2] == 0)
				{
					cout << "single nodeB" << "remainCPU-kep " << purchase_service_info.purchase_service_nodeB_remainCPU[temp_i] << endl;
				}
#endif

				assert(server[0] >= 0 && server[1] >= 0 && server[2] >= 0 && server[3] >= 0);
			}
		}
	}
	return success;
}

// 处理删除虚拟机操作
void delVM(vector<string> &delVmInfo) {

	per_day_VmTotal_info.deployed_Vm_number--;

	string _vmId = delVmInfo[1];

	/*
	vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1,2 };  双节点

	vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1 };  A节点

	*/
	vector<int> _vmInfo = vmOnServer[_vmId];
	int _serverId = _vmInfo[0];

	serverRunVms[_serverId]--;




	vector<int> &server = sysServerResource[_serverId];

	/*
	更新服务器剩余的信息
	*/
#ifdef My_PRINT
	cout << server[2] << endl;
#endif
	server[0] = purchase_service_info.purchase_service_nodeA_remainCPU[_serverId]; //A节点剩余CPU
	server[1] = purchase_service_info.purchase_service_nodeB_remainCPU[_serverId]; //B节点剩余CPU
	server[2] = purchase_service_info.purchase_service_nodeA_remainMEM[_serverId]; //A节点剩余MEM
	server[3] = purchase_service_info.purchase_service_nodeB_remainMEM[_serverId]; //B节点剩余MEM

#ifdef My_PRINT
	cout << server[2] << endl;
#endif

	if (_vmInfo.size() == 5) {
		int cores = _vmInfo[1] / 2, memory = _vmInfo[2] / 2;
		server[0] += cores;
		server[1] += cores;
		server[2] += memory;
		server[3] += memory;
	}
	else {
		int cores = _vmInfo[1], memory = _vmInfo[2];
		if (_vmInfo[3] == 1) {
			server[0] += cores;
			server[2] += memory;
		}
		else {
			server[1] += cores;
			server[3] += memory;
		}
	}

	// 实现string ->转 int
	int vm_tras_ID = 0;
	for (int i_trans = 0; i_trans < _vmId.size(); i_trans++)
	{
		vm_tras_ID = 10 * vm_tras_ID + _vmId[i_trans] - '0';
	}



	/*删除虚拟机负载的信息  默认，load_vm <= 29   39*/
	for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
	{

		if (purchase_service_info.purchase_service_ID_Info[_serverId][3 + 4 * load_vm] == vm_tras_ID)
		{
			purchase_service_info.purchase_service_ID_Info[_serverId][3 + 4 * load_vm] = 0;
			purchase_service_info.purchase_service_ID_Info[_serverId][4 + 4 * load_vm] = 0;   //Nodes
			purchase_service_info.purchase_service_ID_Info[_serverId][5 + 4 * load_vm] = 0;   //CPU
			purchase_service_info.purchase_service_ID_Info[_serverId][6 + 4 * load_vm] = 0;   //MEM

			break;
		}

		if (load_vm > 73)
		{
			for (int iii = 0; iii < 74; iii++)
			{
				cout << "iii:" << iii << "::" << purchase_service_info.purchase_service_ID_Info[_serverId][3 + 4 * iii] << endl;
			}
		}

		if (load_vm == 74)
		{
			cout << "74:serverID:" << _serverId << "    @ 74:IDvm_tras_ID:" << vm_tras_ID << endl;
		}
		assert(load_vm < 74);
	}



#ifdef My_PRINT
	cout << server[2] << endl;
#endif

	//重新更新服务器剩余的信息
	purchase_service_info.purchase_service_nodeA_remainCPU[_serverId] = server[0]; //A节点剩余CPU
	purchase_service_info.purchase_service_nodeB_remainCPU[_serverId] = server[1]; //B节点剩余CPU
	purchase_service_info.purchase_service_nodeA_remainMEM[_serverId] = server[2]; //A节点剩余MEM
	purchase_service_info.purchase_service_nodeB_remainMEM[_serverId] = server[3]; //B节点剩余MEM
}

// 初始化server，如何初始化购买的服务器是一个大的优化
void Init_BuyServer() {

	purchase_service_info.purchased_Service_number = 2600;

	//此处为购买两种类型的服务器  共计2500(2200)台
	/*
	(hostUY41I, 676, 994, 243651, 305)  --->这个测试是ok的，优化过后为1.65左右
	(hostE8YFB, 470, 1016, 199294, 250) --->
	*/
	string serverType = "hostE8YFB";
	int init_ServerNumber = 2600;

	int server_numberID = 0;

	serverRunVms.resize(init_ServerNumber, 0);
	string initBuy = "(purchase, ";
	initBuy += to_string(2) + ")\n";

	//vector<string> res;
	res.push_back(initBuy); //(purchase, 2)
#ifdef My_PRINT
	cout << initBuy << endl;
#endif

	string pauseInfo = "(" + serverType + ", ";
	pauseInfo += std::to_string(init_ServerNumber / 2) + ")\n";

	res.push_back(pauseInfo); //(hostUY41I, 1250)
#ifdef My_PRINT
	cout << pauseInfo << endl;
#endif

#ifdef My_PRINT
	printf("Init BuyServer start\r\n");
#endif

	for (int i = 0; i < init_ServerNumber / 2; i++)
	{

		/*
		serverInfos[_serverType] = vector<int>{ _cpuCores / 2 ,
		_cpuCores / 2,
		_memorySize / 2,
		_memorySize / 2,
		_serverCost,
		_powerCost };
		*/
		//unordered_map<int,vector<int>> sysServerResource;
		//sysServerResource记录购买的服务器
		sysServerResource[serverNumber++] = serverInfos[serverType];
		SERVERCOST += serverInfos[serverType][4];


		//	serverInfos[_serverType] = vector<int>{ _cpuCores / 2 ,
		//_cpuCores / 2,
		//_memorySize / 2,
		//_memorySize / 2,
		//_serverCost,
		//_powerCost };


		//记录购买的虚拟机信息，为后面迁移做准备
		purchase_service_info.purchase_service_ID_Info[server_numberID][0] = server_numberID; //存储服务器节点
		purchase_service_info.purchase_service_nodeA_remainCPU[server_numberID] = serverInfos[serverType][0];
		purchase_service_info.purchase_service_nodeB_remainCPU[server_numberID] = serverInfos[serverType][1];
		purchase_service_info.purchase_service_nodeA_remainMEM[server_numberID] = serverInfos[serverType][2];
		purchase_service_info.purchase_service_nodeB_remainMEM[server_numberID] = serverInfos[serverType][3];

		// 1-->记录总的CPU   2->记录总的MEM
		purchase_service_info.purchase_service_ID_Info[server_numberID][1] = serverInfos[serverType][0] + serverInfos[serverType][1];
		purchase_service_info.purchase_service_ID_Info[server_numberID][2] = serverInfos[serverType][2] + serverInfos[serverType][3];

		server_numberID++;
	}
#ifdef My_PRINT
	printf("Init BuyServer end\r\n");
#endif

	/*
	(host78BMY, 996, 332, 246869, 310)
	(hostGJ11Y, 904, 548, 246070, 308)
	*/
	serverType = "hostGJ11Y";
	pauseInfo = "(" + serverType + ", ";
	pauseInfo += std::to_string(serverNumber) + ")\n";//(host78BMY, 1250)*/

	res.push_back(pauseInfo);
#ifdef My_PRINT
	cout << pauseInfo << endl;
#endif


	for (int i = 0; i < init_ServerNumber / 2; i++)
	{
		sysServerResource[serverNumber++] = serverInfos[serverType];//次数 serverNumber=2500
		SERVERCOST += serverInfos[serverType][4];


		//记录购买的虚拟机信息，为后面迁移做准备
		purchase_service_info.purchase_service_ID_Info[server_numberID][0] = server_numberID; //存储服务器节点
		purchase_service_info.purchase_service_nodeA_remainCPU[server_numberID] = serverInfos[serverType][0];
		purchase_service_info.purchase_service_nodeB_remainCPU[server_numberID] = serverInfos[serverType][1];
		purchase_service_info.purchase_service_nodeA_remainMEM[server_numberID] = serverInfos[serverType][2];
		purchase_service_info.purchase_service_nodeB_remainMEM[server_numberID] = serverInfos[serverType][3];

		// 1-->记录CPU   2->记录MEM
		purchase_service_info.purchase_service_ID_Info[server_numberID][1] = serverInfos[serverType][0] + serverInfos[serverType][1];
		purchase_service_info.purchase_service_ID_Info[server_numberID][2] = serverInfos[serverType][2] + serverInfos[serverType][3];

		server_numberID++;
	}

}

// 扩容服务器策略
void service_expansion() {
	string s = "(purchase, 0)\n";
	res.push_back(s);
#ifdef My_PRINT
	cout << s << endl;
#endif
}

// 迁移虚拟机策略
int vm_migrate()
{

	int no_shift = 1;


	//int count_migration = per_day_VmTotal_info.deployed_Vm_number / 1000;
	int count_migration = 0;
	if (per_day_VmTotal_info.deployed_Vm_number > 2000)
	{
		count_migration = 1;  //先固定迁移的次数   因为首先计算出来的迁移数目不一定都能实现
	}

	//每天可以迁移的虚拟机总量不得超过 5n/1000 向下取整
	if (count_migration >= 1)
	{

		/*
		string s;
		string _migration = "migration";
		s = "(" + _migration + ", ";
		s += std::to_string(count_migration) + ")\n";//(migration, count_migration)

		res.push_back(s);
		#ifdef TEST
		cout << s << endl;
		#endif

		*/

		/*
		遍历所有已购买的服务器，从大号往前面开始查询剩余的CPU和MEM，目前设定如果CPU && MEM均大于0.5f，
		就需要将当前的虚拟机往前面放（目前没有计算前面服务器剩余量，直接往前面遍历)
		然后purchase_service_info.purchase_service_ID_Info[][i]--->i = 0 时，设置的是服务器的编号
		i = 1 时，设置的是服务器编号对应下总的CPU
		i = 2 时，设置的是服务器编号对应下总的MEM
		i = 3-29 时，设置的是搭载虚拟机的编号
		修改一下i= 3 4 5 6表示虚拟机的的编号 单/双节点 CPU MEM
		120 121 122 123 ---30
		*/
		for (int _count = (purchase_service_info.purchased_Service_number - 1); _count >= 0; _count--)
		{
			//if (purchase_service_info.purchase_service_nodeA_remainCPU[_count] >= 0)
			//{   
			float remain_cpu = (purchase_service_info.purchase_service_nodeA_remainCPU[_count]
				+ purchase_service_info.purchase_service_nodeB_remainCPU[_count])*1.0f
				/ purchase_service_info.purchase_service_ID_Info[_count][1] * 1.0f;

			float remain_mem = (purchase_service_info.purchase_service_nodeA_remainMEM[_count]
				+ purchase_service_info.purchase_service_nodeB_remainMEM[_count])*1.0f
				/ purchase_service_info.purchase_service_ID_Info[_count][2] * 1.0f;

			if ((remain_cpu > 0.8f) && (remain_cpu < 0.99f) && (remain_mem > 0.8f) && (remain_mem < 0.99f))
				//if ((remain_cpu > 0.8f) && (remain_mem > 0.8f))
			{
				//for (int vm_tra = 3; purchase_service_info.purchase_service_ID_Info[_count][vm_tra] != 0; vm_tra= vm_tra+4)
				for (int vm_tra = 3; vm_tra <= 299; vm_tra = vm_tra + 4)   //30 -- 119  and 40 --159   60-239  65 259   75  299
				{
					if (purchase_service_info.purchase_service_ID_Info[_count][vm_tra] > 100)
					{
						for (int service_tra = 0; service_tra < 2600; service_tra++)
						{
							if (service_tra != _count)  //服务器不能自己迁移到自己本身
							{
								//if (purchase_service_info.purchase_service_ID_Info[service_tra][vm_tra + 1]) //如果是双节点
								if (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] == 1) //如果是双节点
								{
									if ((purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] >= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2))
										&& (purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] >= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2))
										&& (purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] >= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2))
										&& (purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] >= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2))
										)
									{
#ifdef My_PRINT
										cout << "nodeA_remain cpu:" << purchase_service_info.purchase_service_nodeA_remainCPU[_count] << " 1  " << purchase_service_info.purchase_service_ID_Info[_count][1] << endl;
										cout << "nodeB_remain cpu:" << purchase_service_info.purchase_service_nodeB_remainCPU[_count] << " 1  " << purchase_service_info.purchase_service_ID_Info[_count][1] << endl;
										cout << "nodeA_remain mem:" << purchase_service_info.purchase_service_nodeA_remainMEM[_count] << " 1  " << purchase_service_info.purchase_service_ID_Info[_count][2] << endl;
										cout << "nodeB_remain mem:" << purchase_service_info.purchase_service_nodeB_remainMEM[_count] << " 1  " << purchase_service_info.purchase_service_ID_Info[_count][2] << endl;
#endif

										purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] -= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2);
										purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] -= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2);
										purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] -= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2);
										purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] -= (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2);

										//需要补充选定服务器移出去的虚拟机的CPU和MEM
										purchase_service_info.purchase_service_nodeA_remainCPU[_count] += (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2);
										purchase_service_info.purchase_service_nodeB_remainCPU[_count] += (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] / 2);
										purchase_service_info.purchase_service_nodeA_remainMEM[_count] += (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2);
										purchase_service_info.purchase_service_nodeB_remainMEM[_count] += (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] / 2);

#ifdef My_PRINT
										cout << "nodeA_remain cpu:" << purchase_service_info.purchase_service_nodeA_remainCPU[_count] << " 2  " << purchase_service_info.purchase_service_ID_Info[_count][1] << endl;
										cout << "nodeB_remain cpu:" << purchase_service_info.purchase_service_nodeB_remainCPU[_count] << " 2  " << purchase_service_info.purchase_service_ID_Info[_count][1] << endl;
										cout << "nodeA_remain mem:" << purchase_service_info.purchase_service_nodeA_remainMEM[_count] << " 2  " << purchase_service_info.purchase_service_ID_Info[_count][2] << endl;
										cout << "nodeB_remain mem:" << purchase_service_info.purchase_service_nodeB_remainMEM[_count] << " 2  " << purchase_service_info.purchase_service_ID_Info[_count][2] << endl;
#endif

										//vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,1,2 };

										vmOnServer[std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra])] = vector<int>{
											purchase_service_info.purchase_service_ID_Info[service_tra][0],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3],1,2 };



										assert(purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] >= 0
											&& purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] >= 0
											&& purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] >= 0
											&& purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] >= 0);


										assert((purchase_service_info.purchase_service_ID_Info[_count][1] / 2) >= purchase_service_info.purchase_service_nodeA_remainCPU[_count]
											&& (purchase_service_info.purchase_service_ID_Info[_count][1] / 2) >= purchase_service_info.purchase_service_nodeB_remainCPU[_count]
											&& (purchase_service_info.purchase_service_ID_Info[_count][2] / 2) >= purchase_service_info.purchase_service_nodeA_remainMEM[_count]
											&& (purchase_service_info.purchase_service_ID_Info[_count][2] / 2) >= purchase_service_info.purchase_service_nodeB_remainMEM[_count]);

										string s;
										string _migration = "migration";
										s = "(" + _migration + ", ";
										s += std::to_string(1) + ")\n";//(migration, count_migration)
										res.push_back(s);
#ifdef My_PRINT
										cout << s << endl;
#endif

										s = "(" + std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra]) + ", ";
										s += std::to_string(purchase_service_info.purchase_service_ID_Info[service_tra][0]) + ")\n";//(虚拟机ID, 目的服务器ID)*/

										res.push_back(s);

										//添加移入服务器上虚拟机的信息
										for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
										{
											if (purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] == 0)
											{
												purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra];
												purchase_service_info.purchase_service_ID_Info[service_tra][4 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1];   //Nodes
												purchase_service_info.purchase_service_ID_Info[service_tra][5 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];   //CPU
												purchase_service_info.purchase_service_ID_Info[service_tra][6 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];   //MEM

												break;
											}


										}

										//需要删除从服务器移走虚拟机的信息
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra] = 0;
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] = 0;   //Nodes
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] = 0;   //CPU
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] = 0;   //MEM

#ifdef My_PRINT
										cout << s << endl;
#endif

										//计算功耗需要  扣除服务器的虚拟机数量
										serverRunVms[_count]--;
										serverRunVms[service_tra]++;


										count_migration--;

										//break; //如果判断是可以移植的话，即进行移植，移植完之后 立马进行下一个虚拟机
										return 0;
									}
									else
									{
										//否则判断下一个服务器 CPU和MEM（从小号往大号）
									}
								}
								else if (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] == 2)//需要对A节点点进行选择移植  不需要除/2
								{
									if ((purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] >= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2])
										&& (purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] >= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3]))
									{

										purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] -= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];
										purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] -= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];

										//需要补充选定服务器移出去的虚拟机的CPU和MEM
										purchase_service_info.purchase_service_nodeA_remainCPU[_count] += purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];
										purchase_service_info.purchase_service_nodeA_remainMEM[_count] += purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];


										vmOnServer[std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra])] = vector<int>{
											purchase_service_info.purchase_service_ID_Info[service_tra][0],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3],1 };


										assert(purchase_service_info.purchase_service_nodeA_remainCPU[service_tra] >= 0
											&& purchase_service_info.purchase_service_nodeA_remainMEM[service_tra] >= 0);


										assert(purchase_service_info.purchase_service_ID_Info[_count][1] / 2 >= purchase_service_info.purchase_service_nodeA_remainCPU[_count]
											&& purchase_service_info.purchase_service_ID_Info[_count][2] / 2 >= purchase_service_info.purchase_service_nodeA_remainMEM[_count]);

										string s;
										string _migration = "migration";
										s = "(" + _migration + ", ";
										s += std::to_string(1) + ")\n";//(migration, count_migration)
										res.push_back(s);
#ifdef My_PRINT
										cout << s << endl;
#endif

										s = "(" + std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra]) + ", ";
										string _AA_A = "A";
										s += std::to_string(purchase_service_info.purchase_service_ID_Info[service_tra][0]) + ", ";//(虚拟机ID, 目的服务器ID, A)
										s += _AA_A + ")\n";
										res.push_back(s);

										//添加移入服务器上虚拟机的信息
										for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
										{
											if (purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] == 0)
											{
												purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra];
												purchase_service_info.purchase_service_ID_Info[service_tra][4 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1];   //Nodes
												purchase_service_info.purchase_service_ID_Info[service_tra][5 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];   //CPU
												purchase_service_info.purchase_service_ID_Info[service_tra][6 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];   //MEM

												break;
											}


										}

										//需要删除从服务器移走虚拟机的信息
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra] = 0;
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] = 0;   //Nodes
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] = 0;   //CPU
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] = 0;   //MEM
#ifdef My_PRINT
										cout << s << endl;
#endif

										//计算功耗需要  扣除服务器的虚拟机数量
										serverRunVms[_count]--;
										serverRunVms[service_tra]++;


										count_migration--;

										//break; //如果判断是可以移植的话，即进行移植，移植完之后 立马进行下一个虚拟机
										return 0;
									}
									else
									{
										;
									}
								}
								else if (purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] == 3)//需要对B节点点进行选择移植  不需要除/2
								{
									if ((purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] >= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2])
										&& (purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] >= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3]))
									{


										purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] -= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];
										purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] -= purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];

#ifdef TEST
										cout << purchase_service_info.purchase_service_nodeB_remainCPU[_count] << purchase_service_info.purchase_service_ID_Info[_count][1] << endl;
#endif
										//需要补充选定服务器移出去的虚拟机的CPU和MEM
										purchase_service_info.purchase_service_nodeB_remainCPU[_count] += purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];
										purchase_service_info.purchase_service_nodeB_remainMEM[_count] += purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];


										//vmOnServer[vmId] = vector<int>{ serverId,vmCores,vmMemory,2 };
										vmOnServer[std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra])] = vector<int>{
											purchase_service_info.purchase_service_ID_Info[service_tra][0],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2],
											purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3],2 };


#ifdef TEST
										cout << purchase_service_info.purchase_service_nodeB_remainCPU[_count] << endl;
#endif

										assert(purchase_service_info.purchase_service_nodeB_remainCPU[service_tra] >= 0
											&& purchase_service_info.purchase_service_nodeB_remainMEM[service_tra] >= 0);


										assert(purchase_service_info.purchase_service_ID_Info[_count][1] / 2 >= purchase_service_info.purchase_service_nodeB_remainCPU[_count]
											&& purchase_service_info.purchase_service_ID_Info[_count][2] / 2 >= purchase_service_info.purchase_service_nodeB_remainMEM[_count]);

										string s;
										string _migration = "migration";
										s = "(" + _migration + ", ";
										s += std::to_string(1) + ")\n";//(migration, count_migration)
										res.push_back(s);
#ifdef My_PRINT
										cout << s << endl;
#endif

										s = "(" + std::to_string(purchase_service_info.purchase_service_ID_Info[_count][vm_tra]) + ", ";
										string _BB_B = "B";
										s += std::to_string(purchase_service_info.purchase_service_ID_Info[service_tra][0]) + ", ";//(虚拟机ID, 目的服务器ID, B)
										s += _BB_B + ")\n";
										res.push_back(s);

										//添加移入服务器上虚拟机的信息
										for (int load_vm = 0; load_vm < 75; load_vm++) //因为del的原因，需要解决删除信息的方面
										{
											if (purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] == 0)
											{
												purchase_service_info.purchase_service_ID_Info[service_tra][3 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra];
												purchase_service_info.purchase_service_ID_Info[service_tra][4 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1];   //Nodes
												purchase_service_info.purchase_service_ID_Info[service_tra][5 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2];   //CPU
												purchase_service_info.purchase_service_ID_Info[service_tra][6 + 4 * load_vm] = purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3];   //MEM

												break;
											}


										}

										//需要删除从服务器移走虚拟机的信息
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra] = 0;
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 1] = 0;   //Nodes
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 2] = 0;   //CPU
										purchase_service_info.purchase_service_ID_Info[_count][vm_tra + 3] = 0;   //MEM


#ifdef My_PRINT
										cout << s << endl;
#endif

										//计算功耗需要  扣除服务器的虚拟机数量
										serverRunVms[_count]--;
										serverRunVms[service_tra]++;


										count_migration--;

										//break; //如果判断是可以移植的话，即进行移植，移植完之后 立马进行下一个虚拟机	
										return 0;
									}
									else
									{
										;
									}
								}

								if (count_migration == 0)
								{
									return 0;
								}
							}
							else
							{
								;
							}
						}
					}
					else
					{
						;
					}
				}
				//不移动
				/*
				if (no_shift == 1)
				{
				string s = "(migration, 0)\n";
				res.push_back(s);

				no_shift = 0;
				}
				*/

			}
			else
			{
				//否则判断上一个服务器 CPU和MEM（从大号往小号）
			}

			if (count_migration == 0)
			{
				return 0;
			}
			//}
		}
		//不移动
		string s = "(migration, 0)\n";
		res.push_back(s);
#ifdef My_PRINT
		cout << s << endl;
#endif

	}
	else
	{
		string s = "(migration, 0)\n";
		res.push_back(s);
#ifdef My_PRINT
		cout << s << endl;
#endif
	}

	return 0;
}

/*
分配策略
目前 day=requestdays
*/
void service_vm_match(int day)
{
	if (day != 0)
	{

		//目前打算对每天申请的虚拟机的CPU和内存量来进行服务器的购买
		service_expansion();
		/* 目前service_expansion函数内容，
		string s = "(purchase, 0)\n";
		res.push_back(s);
		*/
	}

	vm_migrate();
	/* 目前vm_migrate函数内容
	string s = "(migration, 0)\n";
	res.push_back(s);
	*/


#ifdef TEST
	//    printf("There are %d requests waiting to matching !!\n",requestInfos.size());
#endif
	for (auto req : requestInfos)
	{				//requestInfos =  {_op, _reqVmType, _reqId}); or {_op, _reqId});
					// 创建虚拟机 还是 删除虚拟机
		int opType = req.size() == 3 ? 1 : 0;

		if (opType)
		{
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

void serverPower()
{
	for (int i = 0; i < serverNumber; i++)
	{
		if (serverRunVms[i] != 0)
		{
			POWERCOST += sysServerResource[i][5];
		}
	}
}


int main() {
	clock_t start, finish;
	start = clock();

	//@@@@@@@@@@@@@@@@@@@@ parameter assignment @@@@@@@@@@@@@@@@@@@@@@
	_server_info_stu server_info_stu;//store   server information
	_vm_info_stu vm_info_stu;      //store  vm information
	_userVm_requestAdd_info userVm_requestAdd_info;
	_userVm_requestDel_info userVm_requestDel_info;

	/*@################需要对vector<int>参量进行数值初始化，否则后面赋值会报错  ######
	目前设置购买的服务器数量ServerNumberALL为2200，
	然后purchase_service_info.purchase_service_ID_Info[][i]--->i = 0 时，设置的是服务器的编号
	i = 1 时，设置的是服务器编号对应下总的CPU
	i = 2 时，设置的是服务器编号对应下总的MEM
	i = 3-29 时，设置的是搭载虚拟机的编号
	修改一下i= 3 4 5 6表示虚拟机的的编号 单/双节点 CPU MEM
	120 121 122 123 ---30
	*/


	for (int init_i = 0; init_i < ServerNumberALL; init_i++)
	{
		purchase_service_info.purchase_service_ID_Info[init_i].resize(303, 0);  //30 -123   and  40 - 163  60 - 243  65 -263    --75  303
	}
	purchase_service_info.purchase_service_nodeA_remainCPU.resize(ServerNumberALL, 0);
	purchase_service_info.purchase_service_nodeB_remainCPU.resize(ServerNumberALL, 0);
	purchase_service_info.purchase_service_nodeA_remainMEM.resize(ServerNumberALL, 0);
	purchase_service_info.purchase_service_nodeB_remainMEM.resize(ServerNumberALL, 0);


	//@@@@@@@@@@@@@@@@@@@@  end @@@@@@@@@@@@@@@@@@@@@@


#ifdef My_PRINT
	printf("Let's begin\r\n");
#endif

	//在读取文件时，采用freopen进行重定向到txt文件，采用cin标准输入读取数据
#ifdef TEST
	std::freopen(filePath.c_str(), "rb", stdin);
#endif
	int serverNum;

	scanf("%d", &serverNum);

	for (int i = 0; i < serverNum; i++)
	{
		cin >> server_info_stu.serverType >> server_info_stu.cpuCores >> server_info_stu.memorySize >> server_info_stu.serverCost >> server_info_stu.powerCost;

		generateServer(&server_info_stu);
	}

	int vmNumber = 0;
	scanf("%d", &vmNumber);



	for (int i = 0; i < vmNumber; i++) {
		cin >> vm_info_stu.vmType >> vm_info_stu.vmCpuCores >> vm_info_stu.vmMemory >> vm_info_stu.vmTwoNodes;

		generateVm(&vm_info_stu);

	}

	int requestdays = 0,
		dayRequestNumber = 0;

	scanf("%d", &requestdays);




	//初始化购买服务器数量 目前init_ServerNumber = 2200
	Init_BuyServer();



	for (int day = 0; day < requestdays; day++)
	{
		scanf("%d", &dayRequestNumber);
		requestInfos.clear();

		for (int i = 0; i < dayRequestNumber; i++)
		{

			cin >> userVm_requestAdd_info.op;

			if (userVm_requestAdd_info.op[1] == 'a')
			{
				//对于添加虚拟机请求 (add, 虚拟机型号, 虚拟机ID)
				cin >> userVm_requestAdd_info.reqVmType >> userVm_requestAdd_info.reqId;

				//userVmRequestAdd(op,reqVmType,reqId);
				userVmRequestAdd(&userVm_requestAdd_info);
			}
			else
			{
				//对于删除虚拟机请求 (del, 虚拟机ID)
				userVm_requestDel_info.op = userVm_requestAdd_info.op;
				cin >> userVm_requestDel_info.reqId;

				//userVmRequestDel(op,reqId);
				userVmRequestDel(&userVm_requestDel_info);
			}
		}
#ifdef TEST
		if (day == 0 || (day + 1) % 100 == 0) {
			printf("The %d day begin matching!!!\n", day + 1);
		}
#endif
		service_vm_match(day);  //目前 day=requestdays


		serverPower();
		//        break;
	}

	fclose(stdin);
	finish = clock();
	TOTALCOST = SERVERCOST + POWERCOST;
//#ifdef UPLOAD
	#ifdef TEST
	for (auto &s : res) std::cout << s;
#endif
#ifdef TEST
	printf("\nusr time: %f s \n", double(finish - start) / CLOCKS_PER_SEC);
	printf("server cost: %lld \npower cost: %lld \ntotal cost: %lld \n", SERVERCOST, POWERCOST, TOTALCOST);
#endif

	return 0;
}
