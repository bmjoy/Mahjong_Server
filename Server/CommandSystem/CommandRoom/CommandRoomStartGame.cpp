﻿#include "CommandHeader.h"
#include "Room.h"
#include "CharacterPlayer.h"

void CommandRoomStartGame::execute()
{
	Room* room = static_cast<Room*>(mReceiver);
	int dice0 = txUtility::randomInt(0, MAX_DICE - 1);
	int dice1 = txUtility::randomInt(0, MAX_DICE - 1);
	std::map<CHAR_GUID, CharacterPlayer*>::const_iterator iterPlayer = room->getPlayerList().begin();
	std::map<CHAR_GUID, CharacterPlayer*>::const_iterator iterPlayerEnd = room->getPlayerList().end();
	for (; iterPlayer != iterPlayerEnd; ++iterPlayer)
	{
		// 先将玩家的准备标记清空
		room->notifyPlayerReady(iterPlayer->first, false);
		// 通知玩家开始游戏
		CommandCharacterStartGame cmdStartGame(CMD_PARAM);
		cmdStartGame.setDice(dice0, dice1);
		mCommandSystem->pushCommand(&cmdStartGame, iterPlayer->second);
	}
}

std::string CommandRoomStartGame::showDebugInfo()
{
	COMMAND_DEBUG(DEBUG_EMPTY);
}