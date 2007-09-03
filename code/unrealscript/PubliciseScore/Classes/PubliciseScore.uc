/** This file was auto-generated by jpp.  You probably want to be editing ./PubliciseScore.uc.jpp instead. **/



class PubliciseScore extends Mutator config(PubliciseScore);

var config bool bShowTime;
var config int UpdateInterval;

defaultproperties {
  bShowTime=True
  UpdateInterval=19
  // UpdateInterval=9
}

var bool Initialized;
var string titleDefault;

function PostBeginPlay() {
 if (Initialized) {
  Log("PubliciseScore.PostBeginPlay(): Already initialised!");
  return;
 }
 Initialized = True;

 titleDefault = ""; // seems title is not set at this time; so we will load it later
 if (Level.Game.IsA('TeamGamePlus')) {
  SetTimer(UpdateInterval,True);
 } else {
  SetTitle("");
 }
}

event Timer() {
 local string text;
 local int redScore, blueScore;
 // local int gameDuration;
 local int timeLeft;
 redScore = TournamentGameReplicationInfo(Level.Game.GameReplicationInfo).Teams[0].Score;
 blueScore = TournamentGameReplicationInfo(Level.Game.GameReplicationInfo).Teams[1].Score;
 // text = " ["$redScore$":"$blueScore$"]";
 text = " ["$redScore$"-"$blueScore$"]";
 // text = " [Red:"$redScore$" Blue:"$blueScore$"]";
 if (bShowTime && TeamGamePlus(Level.Game).TimeLimit>0) {
  // Log("TimeSeconds="$Level.TimeSeconds$" TimeLimit="$TeamGamePlus(Level.Game).TimeLimit$" StartTime="$Level.Game.StartTime$" CountDown="$TeamGamePlus(Level.Game).CountDown$" NetWait="$TeamGamePlus(Level.Game).NetWait$" ElapsedTime="$TeamGamePlus(Level.Game).ElapsedTime);
  // Log("TimeSeconds="$Level.TimeSeconds$" ElapsedTime="$TeamGamePlus(Level.Game).ElapsedTime$" GRI.RemainingTime="$Level.Game.GameReplicationInfo.RemainingTime$" GRI.ElapsedTime="$Level.Game.GameReplicationInfo.ElapsedTime$" GRI.RemainingMinute="$Level.Game.GameReplicationInfo.RemainingMinute$" GRI.SecondCount="$Level.Game.GameReplicationInfo.SecondCount);
  // gameDuration = Level.TimeSeconds - Level.Game.StartTime;
  // gameDuration = TeamGamePlus(Level.Game).ElapsedTime - TeamGamePlus(Level.Game).CountDown;
  // gameDuration = Level.Game.GameReplicationInfo.ElapsedTime;
  // timeLeft = TeamGamePlus(Level.Game).TimeLimit*60 - gameDuration;
  // Log("gameDuration="$gameDuration$" timeLeft="$timeLeft);
  if (TeamGamePlus(Level.Game).ElapsedTime == 0) { // ElapsedTime goes from 0 to 9 during start-game countdown
   // waiting for players
   // text = text $ " Waiting for players...";
   text = ""; // no point showing 0-0 before the game has started!
  } else {
   timeLeft = Level.Game.GameReplicationInfo.RemainingTime;
   if (timeLeft>0) {
    text = text $ " ";
    // if (timeLeft>=60) {
     // text = text $ (timeLeft/60) $ "m";
    // }
    //// Since our Timer is called infrequently, we don't show seconds
    // text = text $ (timeLeft%60) $ "s";
    // text = text $ " left";
    text = text $ (timeLeft/60) $ " minutes left";
   } else {
    if (Level.Game.bGameEnded) {
     text = text $ " Game ended";
    } else {
     text = text $ " Overtime!";
    }
   }
  }
 }
 SetTitle(text);
}

function SetTitle(String newText) {
 // local String serverName;
 // serverName = Level.Game.GameReplicationInfo.ServerName;
 // // serverName = TournamentGameReplicationInfo(Level.Game.GameReplicationInfo).ServerName;
 // Log("PubliciseScore.SetTitle(): current title is \"" $ serverName $ "\".");
 // if (StrEndsWith(serverName,"] ")) {
  // serverName = StrBeforeLast(serverName," [");
 // }
 // if (newText != "") {
  // serverName = serverName $ " [" $ newText $ "] ";
 // }
 // Log("PubliciseScore.SetTitle(): new title is \"" $ serverName $ "\".");
 // Level.Game.GameReplicationInfo.ServerName = serverName;
 if (titleDefault == "") {
  titleDefault = Level.Game.GameReplicationInfo.ServerName;
 }
 // Log("PubliciseScore.SetTitle(): setting server title to \"" $ titleDefault $ newText $ "\".");
 Level.Game.GameReplicationInfo.ServerName = titleDefault $ newText;
}
//===============//
//               //
//  JLib.uc.jpp  //
//               //
//===============//
function int SplitString(String str, String divider, out String parts[256]) {
 // local String parts[256];
 // local array<String> parts;
 local int i,nextSplit;
 i=0;
 while (true) {
  nextSplit = InStr(str,divider);
  if (nextSplit >= 0) {
   // parts.insert(i,1);
   parts[i] = Left(str,nextSplit);
   str = Mid(str,nextSplit+Len(divider));
   i++;
  } else {
   // parts.insert(i,1);
   parts[i] = str;
   i++;
   break;
  }
 }
 // return parts;
 return i;
}
function string GetDate() {
    local string Date, Time;
    Date = Level.Year$"-"$PrePad(Level.Month,"0",2)$"-"$PrePad(Level.Day,"0",2);
    Time = PrePad(Level.Hour,"0",2)$":"$PrePad(Level.Minute,"0",2)$"."$PrePad(Level.Second,"0",2);
    return Date$"-"$Time;
}
function string PrePad(coerce string s, string p, int i) {
    while (Len(s) < i)
        s = p$s;
    return s;
}
function bool StrStartsWith(string s, string x) {
 return (InStr(s,x) == 0);
 // return (Left(s,Len(x)) ~= x);
}
// function bool StrEndsWith(string s, string x) {
 // return (Right(s,Len(x)) ~= x);
// }
function bool StrContains(String s, String x) {
 return (InStr(s,x) > -1);
}
function String StrAfter(String s, String x) {
 local int i;
 i = Instr(s,x);
 return Mid(s,i+Len(x));
}
function string StrAfterLast(string s, string x) {
 local int i;
 i = InStr(s,x);
 if (i == -1) {
  return s;
 }
 while (i != -1) {
  s = Mid(s,i+Len(x));
  i = InStr(s,x);
 }
 return s;
}
function string StrBefore(string s, string x) {
 local int i;
 i = InStr(s,x);
 if (i == -1) {
  return s;
 } else {
  return Left(s,i);
 }
}
function string StrBeforeLast(string s, string x) {
 local int i;
 i = InStrLast(s,x);
 if (i == -1) {
  return s;
 } else {
  return Left(s,i);
 }
}
function int InStrOff(string haystack, string needle, int offset) {
 local int instrRest;
 instrRest = InStr(Mid(haystack,offset),needle);
 if (instrRest == -1) {
  return instrRest;
 } else {
  return offset + instrRest;
 }
}
function int InStrLast(string haystack, string needle) {
 local int pos;
 local int posRest;
 pos = InStr(haystack,needle);
 if (pos == -1) {
  return -1;
 } else {
  posRest = InStrLast(Mid(haystack,pos+Len(needle)),needle);
  if (posRest == -1) {
   return pos;
  } else {
   return pos + Len(needle) + posRest;
  }
 }
}
/*



*/
