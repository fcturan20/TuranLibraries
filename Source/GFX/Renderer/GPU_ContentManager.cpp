#include "GPU_ContentManager.h"

namespace GFX_API {
	GPU_ContentManager::GPU_ContentManager() : GLOBALBUFFERID_BITSET(100), VertexAttribLayoutID_BITSET(100), VertexAttributeID_BITSET(32), RTSLOTSETID_BITSET(32), TRANSFERPACKETID_BITSET(8){

	}

	GPU_ContentManager::~GPU_ContentManager() {
		std::cout << "GFX's GPU Content Manager's destructor is called!\n";
	}

	//ID OPERATIONs

	unsigned int GPU_ContentManager::Create_GlobalBufferID() {
		unsigned int ID = GLOBALBUFFERID_BITSET.GetIndex_FirstFalse() + 1;
		GLOBALBUFFERID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void GPU_ContentManager::Delete_GlobalBufferID(unsigned int ID) {
		GLOBALBUFFERID_BITSET.SetBit_False(ID - 1);
	}
	unsigned int GPU_ContentManager::Create_VertexAttributeID() {
		unsigned int ID = VertexAttributeID_BITSET.GetIndex_FirstFalse() + 1;
		VertexAttributeID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void GPU_ContentManager::Delete_VertexAttributeID(unsigned int ID) {
		VertexAttributeID_BITSET.SetBit_False(ID - 1);
	}
	unsigned int GPU_ContentManager::Create_VertexAttribLayoutID() {
		unsigned int ID = VertexAttribLayoutID_BITSET.GetIndex_FirstFalse() + 1;
		VertexAttribLayoutID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void GPU_ContentManager::Delete_VertexAttribLayoutID(unsigned int ID) {
		VertexAttribLayoutID_BITSET.SetBit_False(ID - 1);
	}
	unsigned int GPU_ContentManager::Create_RTSLOTSETID() {
		unsigned int ID = RTSLOTSETID_BITSET.GetIndex_FirstFalse() + 1;
		RTSLOTSETID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void GPU_ContentManager::Delete_RTSLOTSETID(unsigned int ID) {
		RTSLOTSETID_BITSET.SetBit_False(ID - 1);
	}
	unsigned int GPU_ContentManager::Create_TransferPacketID() {
		unsigned int ID = TRANSFERPACKETID_BITSET.GetIndex_FirstFalse() + 1;
		TRANSFERPACKETID_BITSET.SetBit_True(ID - 1);
		return ID;
	}
	void GPU_ContentManager::Delete_TransferPacketID(unsigned int ID) {
		TRANSFERPACKETID_BITSET.SetBit_False(ID - 1);
	}
	GFX_API::TransferPacket* GPU_ContentManager::Find_TransferPacket_byID(unsigned int PacketID, unsigned int* vector_index) {
		TransferPacket* TP = nullptr;
		for (unsigned int i = 0; i < TRANSFER_PACKETs.size(); i++) {
			TransferPacket* TP = &TRANSFER_PACKETs[i];
			if (TP->ID == PacketID) {
				if (vector_index) {
					*vector_index = i;
				}
				return TP;
			}
		}
		TuranAPI::LOG_ERROR("GPU_ContentManager::Find_TransferPacket_byID() has failed!");
		return nullptr;
	}

	void GPU_ContentManager::Add_to_TransferPacket(unsigned int TransferPacketID, void* TransferHandle) {
		TransferPacket* pack = Find_TransferPacket_byID(TransferPacketID);
		pack->Packet.push_back(TransferHandle);
	}

	//STRUCT FILLING
	unsigned int GPU_ContentManager::Create_VertexAttribute(DATA_TYPE TYPE, bool is_perVertex) {
		VertexAttribute Attribute;
		Attribute.ID = Create_VertexAttributeID();
		Attribute.DATATYPE = TYPE;
		VERTEXATTRIBUTEs.push_back(Attribute);
	}
}