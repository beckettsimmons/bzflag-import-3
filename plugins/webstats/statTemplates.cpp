// statTemplates.cpp : Defines the entry point for the DLL application.
//

#include "statTemplates.h"
#include "bzfsAPI.h"
#include "plugin_utils.h"

double start;
std::string getFileHeader ( void )
{
  start = bz_getCurrentTime();
  std::string page ="<HTML><HEAD></HEAD><BODY>\n<BR>\n";

  std::string publicAddr = bz_getPublicAddr().c_str();
  page = "Stats for ";
  page += publicAddr + "<br>\n";

  return page;
}

std::string getTeamTextName ( bz_eTeamType team )
{
  std::string name = "unknown";

  switch (team)
  {
  case eRedTeam:
    name = "Red";
    break;

  case eGreenTeam:
    name = "Green";
    break;

  case eBlueTeam:
    name = "Blue";
    break;

  case ePurpleTeam:
    name = "Purple";
    break;

  case eRogueTeam:
    name = "Rogue";
    break;

  case eObservers:
    name = "Observers";
    break;

  case eRabbitTeam:
    name = "Rabbit";
    break;
  case eHunterTeam:
    name = "Hunter";
    break;

  }
  return name;
}

std::string getFileFooter ( void )
{
  return format("<br><hr>Page generated by webstats in %f seconds\n",bz_getCurrentTime()-start);
}

std::string getPlayersHeader ( void )
{
  return std::string ("<br><hr><h2>Players</h2><br>");
}

std::string getPlayersFooter ( void )
{
  return std::string ("<br>");
}

std::string getTeamHeader ( bz_eTeamType team )
{
  std::string code = "<br><font";
  code += getTeamFontCode(team);
  code += ">" + getTeamTextName(team) = "</font><br>";

  return code;
}

std::string getTeamFooter ( bz_eTeamType team )
{
  return std::string();
}

std::string getTeamFontCode ( bz_eTeamType team )
{
  std::string code = "";
  switch (team)
  {
  case eRedTeam:
    code = "color=#800000";
    break;

  case eGreenTeam:
    code = "color=#008000";
    break;

  case eBlueTeam:
    code = "color=#000080";
    break;

  case ePurpleTeam:
    code = "color=#800080";
    break;

  case eRogueTeam:
    code = "color=#808000";
    break;

  case eObservers:
    code = "color=#808080";
    break;

  case eHunterTeam:
    code = "color=#C35617";
    break;
  
  case eRabbitTeam:
    code = "color=#C0C0C0";
     break;
  }

  return code;
}

std::string getPlayerLineItemHeader ( bz_BasePlayerRecord *rec )
{
  return std::string ("");
}

std::string getPlayerLineItemFooter ( bz_BasePlayerRecord *rec )
{
  return std::string ("<br>");
}

std::string getPlayerLineItemInterField ( bz_BasePlayerRecord *rec )
{
  return std::string (" ");
}

std::string getPlayerLineItem ( bz_BasePlayerRecord *rec )
{
  std::string code = getPlayerLineItemHeader(rec);

  code += "<font " + getTeamFontCode(rec->team) + ">";
  code += rec->callsign.c_str();
  code += "</font>";

  if ( rec->team != eObservers )
  {
    code += format(" Wins=%d Losses=%d TKs=%d",rec->wins,rec->losses,rec->teamKills);
    if ( rec->admin )
      code += getPlayerLineItemInterField(rec) + "Admin";

    if ( rec->spawned )
      code += getPlayerLineItemInterField(rec)+ "Spawned";

    if ( rec->verified )
      code += getPlayerLineItemInterField(rec) + "Verified";
  }
  code += getPlayerLineItemFooter(rec);

  return code;
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
