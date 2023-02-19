#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <bitset>
#include "renamer.h"
using namespace std;

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches, uint64_t n_active)
{
	//cout<< "\nrenamer construction working";
        //asserts
        assert(n_phys_regs > n_log_regs);
	assert(1<= n_branches);
	assert(n_branches <= 64);
	assert(n_active >0);
	
	logRegs_num = n_log_regs;
	PhyRegs_num = n_phys_regs;
	Unresolved_Branches_num = n_branches;
	ActiveList_size = n_active;

	FreeList_size = PhyRegs_num - logRegs_num;
	
	//Allocating space to RMT, AMT and SMT
	RMT = new unsigned long [logRegs_num];
	AMT = new unsigned long [PhyRegs_num];
	//SMT = new unsigned long* [Unresolved_Branches_num];
	
	//branchcheckpoints
	BrCheckPoint =new Branch_Checkpoints* [Unresolved_Branches_num];
	//BrCheckPoint =new Branch_Checkpoints [Unresolved_Branches_num];
	for(int i=0;i<Unresolved_Branches_num; i++)
	{
                BrCheckPoint[i] = new Branch_Checkpoints;
		BrCheckPoint[i]->SMT = new uint64_t [logRegs_num];
	}
//	{
		//BrCheckPoint[i].SMT = new unsigned long* [logRegs_num];
//		BrCheckPoint[i].GBM =0;
//		BrCheckPoint[i].Head_FreeList =0;

//	}

	GBM=0;
	BranchMask =1;
	//BranchCount=0;	
	
	//Allocate space to free list
	FreeList = new uint64_t [FreeList_size];
	HeadPhaseFreeList = false;
	TailPhaseFreeList = true;
	HeadFreeList = 0;
	TailFreeList = 0;
	//cout<<"\n freelist space allocated";
	//Allocate space to active list
	ActiveList = new Active_List [ActiveList_size];
	HeadPhaseActiveList = false;
	HeadActiveList = 0;
	TailPhaseActiveList = false;
	TailActiveList = 0;

	//initalize AMT, RMT, free list
	int i;
	for(i=0; i<logRegs_num; i++)
	{
		AMT[i]=i;
		RMT[i]=i;
	}
	//int j;
	for(i=0; i<FreeList_size; i++)
	{
		FreeList[i]= logRegs_num +i;
	}

	//Allocate and initalize PRF
	PRF = new uint64_t [PhyRegs_num];
	PRFReadyBit = new bool[PhyRegs_num];
	for(int i=logRegs_num; i<PhyRegs_num; i++)
	{
		//PRF[i]= 0;
		PRFReadyBit[i] = false;
	}

	for(int i= 0; i < logRegs_num; i++)
	{
		PRFReadyBit[i] = true;
	}
/*
	for(int i=logRegs_num; i<PhyRegs_num; i++)
	{
		PRFReadyBit[i] = true;
	}
*/
	for(int i=0; i<ActiveList_size; i++)
	{
		
		ActiveList[i].completed = 0;
		ActiveList[i].exception = 0;
		ActiveList[i].load_violation = 0;
		ActiveList[i].branch_misprediction =0;
		ActiveList[i].isLoad =0;
		ActiveList[i].isStore =0;
		ActiveList[i].isBranch =0;
		ActiveList[i].is_amo =0;
		ActiveList[i].is_csr =0;
		ActiveList[i].value_misprediction =0;
		ActiveList[i].PC =0;
	} 
}

bool renamer::stall_reg(uint64_t bundle_dst)
{
	//cout<<"stall reg function called";
	int num_of_free_regs =0;
	
	if(HeadPhaseFreeList == TailPhaseFreeList)
	{
		num_of_free_regs = TailFreeList - HeadFreeList;
	}
	else
	{
		num_of_free_regs = FreeList_size - HeadFreeList + TailFreeList; 
	}

	if(num_of_free_regs < bundle_dst)
		return true;
	else
		return false;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
	//cout<< "\nstall branch\n";
//	print_al();	

	BranchMask =1;
	int count =0;

	for(int i=0;i<Unresolved_Branches_num; i++)
	{
		if((GBM & BranchMask) == 0)
		{
			count++;
		}
		BranchMask = BranchMask << 1;
	}
	
	BranchMask =1;
	if(count < bundle_branch)
	{
		return true;
	}
	else
		return false;

/*
	uint64_t free_checkpoint =0;
	for(uint64_t j=0; j<Unresolved_Branches_num; j++);
	{
		if((GBM & (1<<j))==0)
		{
			free_checkpoint++;
		}
	}
		
	if(free_checkpoint >= bundle_branch)
	{
		return false;
	}
	else
		return true;
*/
}


uint64_t renamer::get_branch_mask()
{
 	//cout<<"\nbranch getting mask";
	return GBM;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{
	//cout<<"\nrename resource working";
	return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{
	//cout<<"\nrename dest called";
	RMT[log_reg] = FreeList[HeadFreeList];
	HeadFreeList++;
	//printf("\n Headfree list  value :%d",HeadFreeList);
	if (HeadFreeList == FreeList_size)
	{
		HeadFreeList =0;
		phaseFlip(&HeadPhaseFreeList);	
	}
	return RMT[log_reg];
//	printf("RMT[%d] = %d  ",i,RMT[i]);
        
}


 uint64_t renamer::checkpoint()
{
//	cout<<"\ncheckpoint started";	
	unsigned int branchID=0;
	unsigned long long tempGBM=GBM;
	
	BranchMask =1;
	cout<<"Debug: before checkpoint printing GBM"<<endl;
	print_GBM();
        //print_SMT();
	
	for(int i=0; i<Unresolved_Branches_num;i++)
	{
	//	tempGBM &= BranchMask;
		if((tempGBM & 1ULL) == 0)
		{
			//BranchMask = BranchMask >> i;
			branchID = i;

		//	map_copy(RMT, SMT[branchID],logRegs_num);
			for(i=0; i<logRegs_num; i++)
			{
				BrCheckPoint[branchID]->SMT[i] = RMT[i];
		//		printf("\nSMT[%d] = %d",i,BrCheckPoint[branchID]->SMT[i]);	
			}
			cout<<endl;

			BrCheckPoint[branchID]->GBM = GBM;
			GBM = GBM | (BranchMask << branchID);
		//	BrCheckPoint[branchID]->SMT = branchID;	
			BrCheckPoint[branchID]->HeadPhase_FreeList = HeadPhaseFreeList;
			BrCheckPoint[branchID]->Head_FreeList = HeadFreeList ;
		//	BrCheckPoint[branchID]->Branch_count = BranchCount;
		//	BranchCount++;
			break;	
		}
		tempGBM = tempGBM >> 1;
		
			
	}
	cout<<"Debug:checkpoint  Branch ID:"<<branchID<<endl;
	cout<<"Debug: Printing GBM after checkpoint"<<endl;
	print_GBM();
//	cout<<"restored SMT vaues"<<endl;
	
//	cout<<"Checkpoint executed completely";
	//printf("\n branch ID is:%d", branchID);	
	return branchID;
}


bool renamer::stall_dispatch(uint64_t bundle_inst)
{
	//cout<<"stall dispatch called";
	int num_of_Free_Entries = 0;
	
	if(HeadPhaseActiveList == TailPhaseActiveList)
	{
		num_of_Free_Entries = ActiveList_size - TailActiveList + HeadActiveList;
		
	}
	else
	{
		num_of_Free_Entries = HeadActiveList - TailActiveList;

	}

	if(num_of_Free_Entries >= bundle_inst)
	{
		return false;	
	}
	else
	{
		return true;
	}

//	print_al();
		
	//	printf("PRF[%d] = %d  ",i,PRF[i]);	
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg, bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC)
{
	//cout<<"\ninstruction is getting dispatched"; 
    //	assert(ActiveList_size != (ActiveList_size));
	unsigned int index;
	index = TailActiveList;

	ActiveList[TailActiveList].has_Destination = dest_valid;
	if(dest_valid)
	{
		ActiveList[TailActiveList].log_Destination = log_reg;
		ActiveList[TailActiveList].phys_Destination = phys_reg;
	}	

		ActiveList[TailActiveList].isLoad = load;
		ActiveList[TailActiveList].isStore = store;
		ActiveList[TailActiveList].isBranch = branch;
		ActiveList[TailActiveList].is_amo = amo;
		ActiveList[TailActiveList].is_csr = csr;
		ActiveList[TailActiveList].PC = PC ;


		ActiveList[TailActiveList].completed = false;
		ActiveList[TailActiveList].exception = false;
		ActiveList[TailActiveList].load_violation = false;
		ActiveList[TailActiveList].branch_misprediction = false;
		ActiveList[TailActiveList].value_misprediction = false;

		TailActiveList++;
		if(TailActiveList == ActiveList_size)
		{
			TailActiveList = 0;
			phaseFlip(&TailPhaseActiveList);
		}

		//print_al();
		//cout<<"Debug: AL index returned is: "<<index<<endl;
		return index;
}

bool renamer::is_ready(uint64_t phys_reg)
{
	//cout<<"is_ready function started";
	//cout<<"Debug: phys_reg is the reg ready: "<<phys_reg<<endl;
	return PRFReadyBit[phys_reg];
}

void renamer::clear_ready(uint64_t phys_reg)
{
	//cout<<"clear_ready started";
	//cout<<"Debug: phys_reg ready bit clear: "<<phys_reg<<endl;
	PRFReadyBit[phys_reg]=false;
}


void renamer::set_ready(uint64_t phys_reg)
{
	//cout<<"set ready started";
	//cout<<"Debug: phys_reg ready bit set: "<<phys_reg<<endl;
	PRFReadyBit[phys_reg]=true;	
}

uint64_t renamer::read(uint64_t phys_regs)
{
	//cout<<"read started for PRF";
	//cout<<"Debug: phys_reg read: "<<phys_regs<<endl;
	return PRF[phys_regs];	
}
void renamer::write(uint64_t phys_reg, uint64_t value)
{
	//cout<<"write started";
	PRF[phys_reg]=value;
}

void renamer::set_complete(uint64_t AL_index)
{
	//cout<<"set_complete started";
	//cout<<"Debug: AL index to be set as complete: "<<AL_index<<endl;
	ActiveList[AL_index].completed=true;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct)
{
	cout<<"Debug: resolving for Branch ID:"<<branch_ID<<endl;
	cout<<"Debug: printing GBM before resolve"<<endl;
	print_GBM();
//	cout<<"resolved function started";	
	if(correct == true) 
	{
		GBM = GBM & (~(1ULL<< branch_ID));
		for(int i =0; i< Unresolved_Branches_num; i++)
		{
			uint64_t tempGBM;
			tempGBM = BrCheckPoint[i]->GBM;
			tempGBM &= ~(1ULL << branch_ID);

			BrCheckPoint[i]->GBM = tempGBM;
		}
	//	BranchCount--;
	//	BranchMask = 1;
	
//		return;
	}
	else
	{
		cout<<"Debug: Branch is mispredicted"<<endl;
		//assert((GBM & (1<<branch_ID)) > 0);
		GBM = BrCheckPoint[branch_ID]->GBM;
		//restore GBM
		print_GBM();
	//	GBM = GBM & (~(1<< branch_ID)) ;
		//RMT restore karna hai

	//	map_copy(SMT[branch_ID], RMT, logRegs_num);
		for(uint64_t i=0; i<logRegs_num; i++)
		{
			RMT[i] = BrCheckPoint[branch_ID]->SMT[i];
		}
		//BranchCount = BrCheckPoint[branch_ID].Branch_count;

		//freelist restore
		HeadFreeList = BrCheckPoint[branch_ID]->Head_FreeList;
		HeadPhaseFreeList = BrCheckPoint[branch_ID]->HeadPhase_FreeList;

		
			if (AL_index == ActiveList_size-1)
			{
				TailActiveList =0;	
				if(HeadPhaseActiveList == 1)
				{	
				TailPhaseActiveList = 0;
				}
				else 
				TailPhaseActiveList =1;
			}	

			else
			{		
				TailActiveList = AL_index + 1;
				if(AL_index < HeadActiveList) 
				{
					if(HeadPhaseActiveList == 1)
					{
						TailPhaseActiveList = 0;
					}
					else
						TailPhaseActiveList = 1;
				}
				else if (AL_index >= HeadActiveList)
				{
					TailPhaseActiveList = HeadPhaseActiveList;

				} 
			}		

//		cout<<"resolved function is finished";
//		printf("branch Id is %d",branch_ID);
	//	print_al();
	//	return;
	}
//	BranchMask =1;
		cout<<"Debug : Printing GBM after resolved"<<endl;
		print_GBM();
}

bool renamer::precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, bool &val_misp, bool &load, bool &store, bool &branch, bool &amo, bool &csr, uint64_t &PC)
{
		//cout<<"precommit is started";
		
		completed = ActiveList[HeadActiveList].completed;
		exception = ActiveList[HeadActiveList].exception;
		load_viol = ActiveList[HeadActiveList].load_violation;
		br_misp   = ActiveList[HeadActiveList].branch_misprediction;
		load      = ActiveList[HeadActiveList].isLoad;
		store     = ActiveList[HeadActiveList].isStore;
		branch    = ActiveList[HeadActiveList].isBranch;
		amo       = ActiveList[HeadActiveList].is_amo;
		csr       = ActiveList[HeadActiveList].is_csr;
		val_misp  = ActiveList[HeadActiveList].value_misprediction;
		PC        = ActiveList[HeadActiveList].PC;

			
		if(HeadActiveList == TailActiveList && HeadPhaseActiveList == TailPhaseActiveList)
			return false;
		
		else
		{
	/*	completed = ActiveList[HeadActiveList].completed;
		exception = ActiveList[HeadActiveList].exception;
		load_viol = ActiveList[HeadActiveList].load_violation;
		br_misp   = ActiveList[HeadActiveList].branch_misprediction;
		load      = ActiveList[HeadActiveList].isLoad;
		store     = ActiveList[HeadActiveList].isStore;
		branch    = ActiveList[HeadActiveList].isBranch;
		amo       = ActiveList[HeadActiveList].is_amo;
		csr       = ActiveList[HeadActiveList].is_csr;
		val_misp  = ActiveList[HeadActiveList].value_misprediction;
		PC        = ActiveList[HeadActiveList].PC;
	*/
		return true;
                //printf("\nvalues of precommit\n");
		}	
}

void renamer::commit()
{
	//cout<<"\ncommit function started";
	//assert(ActiveList_size !=0);
	//assert(ActiveList[HeadActiveList].completed == 1);
	//assert(ActiveList[HeadActiveList].exception == 0);
	//assert(ActiveList[HeadActiveList].load_violation == 0);
	//assert(ActiveList[HeadActiveList].branch_misprediction == 0);
  	int al_head = HeadActiveList;
	if(ActiveList[HeadActiveList].has_Destination == 1)
	{
		FreeList[TailFreeList] = AMT[ActiveList[HeadActiveList].log_Destination];
		AMT[ActiveList[HeadActiveList].log_Destination] = ActiveList[HeadActiveList].phys_Destination;
		//prf ready bit [freelist[tailFreeList] =0;
		PRFReadyBit[FreeList[TailFreeList]]=0;
		//prfRB[phys_des]=1;
		PRFReadyBit[ActiveList[HeadActiveList].phys_Destination] =1;

		if(TailFreeList == FreeList_size - 1)
		{
			TailFreeList =0;
			phaseFlip(&TailPhaseFreeList);
		}	
		else
		TailFreeList++;

		if(HeadActiveList == ActiveList_size-1)
	 	{
	  		HeadActiveList = 0;
	  		phaseFlip(&HeadPhaseActiveList);
		}
		else 
          		HeadActiveList++;
	}
	else 
	{
		if(HeadActiveList == ActiveList_size-1)
	 	{
	  		HeadActiveList = 0;
	  		phaseFlip(&HeadPhaseActiveList);
		}
		else 
          		HeadActiveList++;
	}
	//cout<<"\ncommit function finished";
//	printf("\nprint AL after commit");
	commit_counter++;
//	printf("\nnumber of commit:%d  at Al_head :%d",commit_counter, al_head);
//	cout<<"\nPrinting GBM after commit"<<endl;
//	print_GBM();
//	print_al();
}

void renamer::squash()
{       
	//printf("\nsquash started");
	map_copy(AMT, RMT, logRegs_num);
	HeadFreeList = TailFreeList;
	HeadPhaseFreeList =! TailPhaseFreeList;

	HeadActiveList = TailActiveList;
	HeadPhaseActiveList = TailPhaseActiveList;
	
	
	GBM =0;
 	//cout<<"\nsquash function ended";       
}

void renamer::set_exception(uint64_t AL_index)
{
	//cout<<"\nset_exception is set";
	ActiveList[AL_index].exception=1;
}

void renamer::set_load_violation(uint64_t AL_index)
{
	//cout<<"\nload_violation function is set";
	ActiveList[AL_index].load_violation=1;
}

void renamer::set_branch_misprediction(uint64_t AL_index)
{
	//cout<<"\nbranch misp set";
	ActiveList[AL_index].branch_misprediction=1;
}

void renamer::set_value_misprediction(uint64_t AL_index)
{
	//cout<<"value misprediction is set";
	ActiveList[AL_index].value_misprediction= 1;
}

bool renamer::get_exception(uint64_t AL_index)
{
	//cout<<"get_expection started";
	return(ActiveList[AL_index].exception=true);
}
