
/* WARNING! This file was auto-generated by jpp.  You probably want to be editing ./ActorEditor.uc.jpp instead. */

// vim: tabstop=2 shiftwidth=2 noexpandtab filetype=uc

// Can we set/change location of objects?

// TODO: things to match (logical combinations of (map-regexp) or (game-variable)
//       things to change: add mutator, change setting of class, change setting of map, spawn object at position

// e.g.: on CTF-LuciusCB set FlagBase1.DrawScale 2.5
// on <condition> do <action>
// action = set <actor>.<varname> <newvalue> | remove <actor> | removeall <actor_type> | ...

// TODO: ActorEditor only works on actors which exist at (before) the start of the game.  (It never acts on an sgBaseCore for example.)
//       Maybe we should check whether to act on Actors at CheckReplacement() stage.

class ActorEditor expands Mutator config(ActorEditor);

// Wizard may want to: replace a specific actor, replace all actors matching "...", change properties of one/more actors

// TODO CONSIDER: collect a list of (max 10?) favourite properties; use these for auto-searching, or display them all on "!examine"

// TODO: to make the config easier to edit, put commands one-per-line, space-delimited (and execute them like normal messages? -- mmm the "SET" command currently requires "LOOK" or "SEARCH" first.)

// DONE: make the FINDCLOSEST function, which matches Actor name, but takes the closest one

// DONE: instead of mixing up SET and STORE commands, save all SETs, but don't SaveConfig() until "SAVEALL" is called.

// TODO: it seems the screen actors get initialised before this mutator is called
//       therefore sometimes they start up displaying their OLD values, until their refresh comes around
//       maybe taking action during CheckReplacement() or AlwaysKeep() could fix this.

// TODO: Screen support should not look for the closest ScreenSlidePage actor.
//       It should find the screen in front of the player, and then check which slide that is pointing at.

// Remove this to leave out Screen support (and Screen dependency).


var config bool bAcceptSpokenCommands;
var config bool bAcceptMutateCommands;
var config bool bSwallowSpokenCommands;
var bool bOnlyAdmin; // TODO: config

var bool bLetPlayersSetScreens; // TODO: config


var config bool bTurnToActor;
var config bool bJumpToActor;
var config float JumpDistance;

var config String UpdateActor[1024];
var config String UpdateProperty[1024];
var config String UpdateValue[1024];

// var config String SwapActor[1024];
// var config String SwapWith[1024];

defaultproperties {
  bAcceptSpokenCommands=True
  bAcceptMutateCommands=True
  bSwallowSpokenCommands=True // Note: only hides the message for the calling player, not the other players.
  bOnlyAdmin=True
  bTurnToActor=True
  bJumpToActor=True
  JumpDistance=120

  bLetPlayersSetScreens=True

}

var Actor workingActor;

function PostBeginPlay() {
 local Actor A;

 // If we were not added as a mutator, but run in some other way (e.g. as a ServerActor), then we need to register as a mutator:
   // Level.Game.BaseMutator.AddMutator(Self);

 // Register to receive spoken messages in MutatorTeamMessage() below:
 if (bAcceptSpokenCommands) {
  Level.Game.RegisterMessageMutator(Self);
 }

 // Update all edits from config:
 foreach AllActors(class'Actor', A) {
  CheckActionOn(A);
 }

}

function CheckActionOn(Actor A) {
 local int i;
 local string searchStr;
 searchStr = Caps(""$A);
 for (i=0;i<1024;i++) {

  // An optimization when using AlwaysKeep, but with a drawback:
  // It breaks if we empty earlier unwanted records.
  /* if (UpdateActor[i] == "") {

			break;

		} */
  if ( UpdateActor[i] ~= searchStr ) {
   Log("[ActorEditor] Updating "$A$"."$UpdateProperty[i]$" -> "$UpdateValue[i]);
   DoSetProperty(A,UpdateProperty[i],UpdateValue[i]);
  }
 }
 // Log("[ActorEditor] Done: "$A);
}
function bool CheckReplacement(Actor Other, out byte bSuperRelevant) {
 local bool bRes;
 bRes = Super.CheckReplacement(Other,bSuperRelevant);
 if (!bRes)
  Log("ActorEditor.CheckReplacement("$Other$","$bSuperRelevant$") = "$bRes);
 return bRes;
}
// Catch messages from players:
function bool MutatorTeamMessage(Actor Sender, Pawn Receiver, PlayerReplicationInfo PRI, coerce string Msg, name Type, optional bool bBeep) {
 local bool hideMessage;
 hideMessage = False;
 if (Sender == Receiver && Sender.IsA('PlayerPawn')) { // Only process each message once.
  if (StrStartsWith(Msg,"!")) {
   hideMessage = CheckMessage(Mid(Msg,1), PlayerPawn(Sender));
  }
 }
 return Super.MutatorTeamMessage(Sender,Receiver,PRI,Msg,Type,bBeep) && (!hideMessage || !bSwallowSpokenCommands);
}
function Mutate(String str, PlayerPawn Sender) {
 if (bAcceptMutateCommands) {
  CheckMessage(str, Sender);
 }
 Super.Mutate(str, Sender);
}
// Returns True if the command was recognised (and therefore the player's message could optionally be swallowed).
function bool CheckMessage(String line, PlayerPawn Sender) {
 local int argCount;
 local String args[256];
 local Actor A;
 local String result;
 local int i,j;
 local String squishedName;
 local String url;
 local String rebuilt_string; // CONSIDER: instead of rebuilding the string, we could just use StrAfter(line," ") one or more times.
 local String command;
 // Log("ActorEditor.CheckMessage() ("$Sender$"): "$Msg$"");
 argcount = SplitString(line," ",args);
 command = args[0];
 if (bLetPlayersSetScreens || Sender.bAdmin || (!bOnlyAdmin)) {
  // Specifically for Screen actors + teleporters.
  if (command ~= "SETSERVER") {
   rebuilt_string = ""; for (i=2;i<argCount;i++) { rebuilt_string = rebuilt_string $ args[i]; if (i<argCount-1) { rebuilt_string = rebuilt_string $ " "; } }
   if (args[1] == "" || rebuilt_string == "") {
    Sender.ClientMessage("Usage: !setserver <server_ip> <server_title/description>");
   } else {
    SetServer(Sender,args[1],rebuilt_string);
    Sender.ClientMessage("Screen server updated.");
   }
   return True;
  }
  if (command ~= "SETTEXT") {
   rebuilt_string = ""; for (i=1;i<argCount;i++) { rebuilt_string = rebuilt_string $ args[i]; if (i<argCount-1) { rebuilt_string = rebuilt_string $ " "; } }
   if (args[1] == "") {
    Sender.ClientMessage("Usage: !settext <new_text>");
   } else {
    A = FindClosestActorMatching(Sender,"ScreenSlidePage");
    SaveUpdate(A,"Text",rebuilt_string);
    Sender.ClientMessage("Screen text updated.");
   }
   return True;
  }
  if (command ~= "SETURL") {
   rebuilt_string = ""; for (i=1;i<argCount;i++) { rebuilt_string = rebuilt_string $ args[i]; if (i<argCount-1) { rebuilt_string = rebuilt_string $ " "; } }
   url = rebuilt_string;
   if (!StrContains(url,"://"))
    args[1] = "http://" $ args[1];
   if (!isURL(url)) {
    Sender.ClientMessage("Usage: !seturl <url>");
   } else {
    A = FindClosestActorMatching(Sender,"Teleporter");
    if (!StrContains(Teleporter(A).URL,"://")) {
     Sender.ClientMessage("This local "$A$" may not be modified.");
    } else {
     SaveUpdate(A,"URL",url);
     Sender.ClientMessage(A$" updated.");
    }
    A = FindClosestActorMatching(Sender,"ScreenSlidePageWeb");
    if (A != None) {
     SaveUpdate(A, "AddressHost", StrBeforeFirst(StrBeforeFirst(StrAfterFirst(url,"://"),":"),"/") );
     SaveUpdate(A, "AddressPort", "80");
     SaveUpdate(A, "AddressPath", "/" $ StrAfterFirst(StrAfterFirst(url,"://"),"/") );
     SaveUpdate(A, "Text", "<p align=center><font color=gray>[Loading "$url$" ...]</font></p>");
     Sender.ClientMessage(A$" updated.");
    }
   }
   return True;
  }
 }
 // Stuff players may or may not be allowed to do:
 if (bOnlyAdmin && !Sender.bAdmin) {
  return False;
 }
 if (command ~= "HELP") {
  Sender.ClientMessage("ActorEditor commands:");
  Sender.ClientMessage("  help | look | search/find/seek <actor_name_part> | findclosest <actor_name_part> | searchprop/findprop/seekprop <property> <value_part> | ");
  // Sender.ClientMessage("  get/check <property> | set/put <property> <value> | store/save <property> <value> | grab <property>");
  Sender.ClientMessage("  get/check <property> | set/put/save/store <property> <value> | grab <property> | setscale <float>");
  Sender.ClientMessage("  setserver <server_address> <server_title/description> | settext <new_text_for_screen> | seturl <new_url_for_teleporter>");
  Sender.ClientMessage("  saveconf <actor_name_part> | saveall");
  // Sender.ClientMessage("  ev"$"ent <tag>");
  return True;
 }
 // if (command ~= ("EV"$"ENT")) {
  // foreach AllActors( class 'Actor', A, args[1]) {
   // A.Trigger( Self, Self.Instigator );
  // }
 // }
 if (command ~= "SHOW" || command ~= "SHOWINVIS" || command~="LIGHT") {
  ShowInvisibleActors();
  return True;
 }
 if (command ~= "LOOK") {
  workingActor = FindClosestActor(Sender);
  Sender.ClientMessage("Viewing: "$workingActor);
  return True;
 }
 if (command ~= "SEARCH" || command ~= "FIND" || command ~= "SEEK") {
  workingActor = SeekActorMatching(args[1],Sender);
  if (workingActor != None) {
   JumpToActor(Sender,workingActor);
  }
  return True;
 }
 if (command ~= "SEARCHLAST" || command ~= "FINDLAST" || command ~= "SEEKLAST") {
  workingActor = SeekLastActorMatching(args[1],Sender);
  return True;
 }
 if (command ~= "FINDCLOSEST") {
  workingActor = FindClosestActorMatching(Sender,args[1]);
  Sender.ClientMessage("Found: "$workingActor);
  return True;
 }
 if (command ~= "SEARCHPROP" || command ~= "FINDPROP" || command ~= "SEEKPROP") {
  workingActor = FindActorWithMatchingProperty(args[1],args[2]);
  Sender.ClientMessage("Found: "$workingActor$" with "$args[1]$": "$workingActor.GetPropertyText(args[1]));
  return True;
 }
 if (command ~= "GET" || command ~= "CHECK" || command ~= "SHOW") {
  result = workingActor.GetPropertyText(args[1]);
  Sender.ClientMessage(workingActor $ "." $ args[1] $ ": " $ result);
  return True;
 }
 if (command ~= "SET" || command ~= "PUT" || command ~= "STORE" || command ~= "SAVE") {
  rebuilt_string = ""; for (i=2;i<argCount;i++) { rebuilt_string = rebuilt_string $ args[i]; if (i<argCount-1) { rebuilt_string = rebuilt_string $ " "; } }
  SaveUpdate(workingActor,args[1],rebuilt_string);
  result = workingActor.GetPropertyText(args[1]);
  Sender.ClientMessage(workingActor $ "." $ args[1] $ " => " $ result);
  // If STORE or SAVE, then we could: SaveConfig();
  return True;
 }
 if (command ~= "GRAB") {
  // Copies the current value of the property into the config file, so it can be edited there.
  result = workingActor.GetPropertyText(args[1]);
  SaveUpdate(workingActor,args[1],result);
  // We could: SaveConfig();
  Sender.ClientMessage(workingActor $ "." $ args[1] $ " << " $ result);
  return True;
 }
 if (command ~= "SETSCALE") {
  Sender.ClientMessage(workingActor $".DrawScale was "$ workingActor.DrawScale);
  workingActor.default.DrawScale = Float(args[1]);
  workingActor.DrawScale = Float(args[1]);
  Sender.ClientMessage(workingActor $".DrawScale is now "$ workingActor.DrawScale);
  return True;
 }
 /*if (command ~= "SETSPRITESCALE") {

		Sender.ClientMessage(sgBuilding(workingActor) $".SpriteScale was "$ sgBuilding(workingActor).SpriteScale);

		sgBuilding(workingActor).default.SpriteScale = Float(args[1]);

		sgBuilding(workingActor).SpriteScale = Float(args[1]);

		Sender.ClientMessage(sgBuilding(workingActor) $".SpriteScale is now "$ sgBuilding(workingActor).SpriteScale);

		return True;

	}*/
 if (command ~= "SAVEALL") {
  SaveConfig();
  Sender.ClientMessage("Your changes have been saved.");
  return True;
 }
 // Stuff really only for admin can do:
 if (!Sender.bAdmin)
  return False;
 if (command ~= "SAVECONF") {
  workingActor = SeekActorMatching(args[1],Sender);
  if (workingActor != None) {
   workingActor.SaveConfig();
   Sender.ClientMessage("ActorEditor forced "$ workingActor $" to save its config.");
  }
  return True;
 }
 return False;
}
function ShowInvisibleActors() {
 local Actor A;
 foreach AllActors(Class'Actor',A) {
  if (A.DrawType == DT_None) {
   A.DrawType = DT_Sprite;
   // A.Sprite = None;
  }
 }
}
function Actor SeekActorMatching(string str, PlayerPawn Sender) {
 local Actor A,F;
 local String others;
 foreach AllActors(class'Actor', A) {
  if (StrContains(Caps(""$A),Caps(str))) {
   // if (F==None || FRand()<0.2) {
   if (F==None) {
    F = A;
   } else {
    if (Len(others) < FRand()*100) {
     others = others $ " " $ ShortActorName(A);
    }
   }
  }
 }
 if (others != "") others = " (others:"$others$")";
 Sender.ClientMessage("Found: "$ ShortActorName(F) $ others);
 return F;
}
function Actor SeekLastActorMatching(string str, PlayerPawn Sender) {
 local Actor A,F;
 local String others;
 foreach AllActors(class'Actor', A) {
  if (StrContains(Caps(""$A),Caps(str))) {
   if (F != None)
    others = others $ " " $ ShortActorName(F);
   F = A;
  }
 }
 if (others != "") others = " (others:"$others$")";
 Sender.ClientMessage("Found: "$ ShortActorName(F) $ others);
 return F;
}
function String ShortActorName(Actor A) {
 return StrAfterFirst(String(A),".");
}
function SaveUpdate(Actor A, String property, String newValue) {
 local int i;
 if (A == None) {
  return;
 }
 DoSetProperty(A,property,newValue);
 for (i=0;i<1024;i++) {
  // Find first empty record, or matching record to replace:
  if (UpdateActor[i] == "" || (UpdateActor[i]~=(""$A) && UpdateProperty[i]~=property)) {
   UpdateActor[i] = ""$A;
   UpdateProperty[i] = property;
   UpdateValue[i] = newValue;
   // SaveConfig();
   break;
  }
 }
}
function DoSetProperty(Actor A, String property, String newValue) {
 local String tmp;
 // Works on most things (including some vectors!)
 A.SetPropertyText(property,newValue);
 Log("ActorEditor.DoSetProperty("$A$",\""$property$"\",\""$newValue$"\") was called.");
 // We cannot set Location the normal way because it's a const; we must use SetVelocity().
 if (property ~= "Location") {
  A.SetLocation( vector(newValue) );
 }
}
function JumpToActor(PlayerPawn P, Actor A) {
 if (bTurnToActor) {
  P.SetRotation( Rotator(A.Location - P.Location) );
 }
 if (bJumpToActor) {
  P.SetLocation( A.Location - vector(P.ViewRotation) * JumpDistance );
 }
}
// Actually specific for my setup, which is 1 Teleporter, 1 ScreenSlidePageServer (which doesn't work) and 1 ScreenSlidePageWeb (which does work).
function SetServer(Actor Sender, String server_ip, String server_description) {
 local Actor A;
 A = FindClosestActorMatching(Sender,"Teleporter");
 SaveUpdate(A,"URL","unreal://"$server_ip);
 A = FindClosestActorMatching(Sender,"ScreenSlidePageServer");
 if (StrContains(server_ip,":")) {
  SaveUpdate(A,"AddressServer",StrBefore(server_ip,":"));
  SaveUpdate(A,"AddressPort",""$(Int(StrAfter(server_ip,":"))+1));
 } else {
  SaveUpdate(A,"AddressServer",server_ip);
  SaveUpdate(A,"AddressPort","7778");
 }
 SaveUpdate(A,"Text","<p align=center><font color=gray size=+3>"$server_description$"</font></p>["$server_ip$"]");
 A = FindClosestActorMatching(Sender,"ScreenSlidePageWeb");
 SaveUpdate(A,"AddressHost","neuralyte.org");
 SaveUpdate(A,"AddressPort","80");
 // SaveUpdate(A,"AddressPath","/~joey/utportal/"$shortWebName$".html");
 // SaveUpdate(A,"Text","<p align=center><font color=gray>[No "$shortWebName$".html from website]</font></p>["$server_ip$"]");
 SaveUpdate(A,"AddressPath","/cgi-bin/utq?ip="$server_ip);
 SaveUpdate(A,"Text","<p align=center><font color=gray>[Waiting to access web CGI script...]</font></p>["$server_ip$"]");
}
function bool isURL(String str) {
 return (InStr(str,"://")>=0 && InStr(str,"://")<50);
}
function Actor FindClosestActor(Actor from) {
 local Actor A;
 local int distance;
 local int deltaRotation;
 local Actor bestActor;
 local int bestDistance;
 bestActor = None;
 foreach VisibleActors(class'Actor', A, 1024, from.Location) {
 // foreach AllActors(class'Actor', A) { // not using VisibleActors gets us more invisible actors like InventorySpot/Light/...
  if (A == from) { // don't find self!
   continue;
  }
  distance = VSize(A.Location - from.Location);
  deltaRotation = Abs( Rotator(A.Location - from.Location).Yaw - from.Rotation.Yaw ) % 65536;
  // if (deltaRotation < 8192 || deltaRotation > 8192*7) {
  if (deltaRotation > 8192*4) {
   deltaRotation = 8192*8 - deltaRotation;
  }
  if (deltaRotation < 8192 && deltaRotation > -8192) {
   if (bestActor == None || distance < bestDistance) {
    bestActor = A;
    bestDistance = distance;
    // PlayerPawn(from).ClientMessage("  " $ A $" (" $ deltaRotation $ ") -> " $ distance $ "");
   }
  }
 }
 return bestActor;
}
function Actor FindMatchingActor(string str) {
 local Actor A;
 foreach AllActors(class'Actor', A) {
  if (StrContains(Caps(""$A),Caps(str))) {
   return A;
  }
 }
 return None;
}
function Actor FindClosestActorMatching(Actor from, String str) {
 local Actor A;
 local int distance;
 local int deltaRotation;
 local Actor bestActor;
 local int bestDistance;
 bestActor = None;
 // foreach VisibleActors(class'Actor', A, 1024, from.Location) {
 foreach AllActors(class'Actor', A) { // not using VisibleActors gets us more invisible actors like InventorySpot/Light/...
  if (A == from) { // don't find self!
   continue;
  }
  if (!StrContains(Caps(""$A),Caps(str))) {
   continue;
  }
  distance = VSize(A.Location - from.Location);
  deltaRotation = Abs( Rotator(A.Location - from.Location).Yaw - from.Rotation.Yaw ) % 65536;
  // if (deltaRotation < 8192 || deltaRotation > 8192*7) {
  if (deltaRotation > 8192*4) {
   deltaRotation = 8192*8 - deltaRotation;
  }
  if (deltaRotation < 8192 && deltaRotation > -8192) {
   if (bestActor == None || distance < bestDistance) {
    bestActor = A;
    bestDistance = distance;
    // PlayerPawn(from).ClientMessage("  " $ A $" (" $ deltaRotation $ ") -> " $ distance $ "");
   }
  }
 }
 return bestActor;
}
function Actor FindClosestActorClass(Actor from, class<Actor> cls) {
 local Actor A;
 local int distance;
 local int deltaRotation;
 local Actor bestActor;
 local int bestDistance;
 bestActor = None;
 // foreach VisibleActors(class'Actor', A, 1024, from.Location) {
 // foreach AllActors(class'Actor', A) { // not using VisibleActors gets us more invisible actors like InventorySpot/Light/...
 foreach AllActors(cls, A) { // not using VisibleActors gets us more invisible actors like InventorySpot/Light/...
  if (A == from) { // don't find self!
   continue;
  }
  if (A.class != cls) {
  // if (!A.IsA(cls)) {
   continue;
  }
  distance = VSize(A.Location - from.Location);
  deltaRotation = Abs( Rotator(A.Location - from.Location).Yaw - from.Rotation.Yaw ) % 65536;
  // if (deltaRotation < 8192 || deltaRotation > 8192*7) {
  if (deltaRotation > 8192*4) {
   deltaRotation = 8192*8 - deltaRotation;
  }
  if (deltaRotation < 8192 && deltaRotation > -8192) {
   if (bestActor == None || distance < bestDistance) {
    bestActor = A;
    bestDistance = distance;
    // PlayerPawn(from).ClientMessage("  " $ A $" (" $ deltaRotation $ ") -> " $ distance $ "");
   }
  }
 }
 return bestActor;
}
function Actor FindActorWithMatchingProperty(string prop, string val) {
 local Actor A;
 foreach AllActors(class'Actor', A) {
  if ( StrContains(Caps(A.GetPropertyText(prop)),Caps(val)) ) {
   return A;
  }
 }
 return None;
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
 Date = Level.Year$"/"$PrePad(Level.Month,"0",2)$"/"$PrePad(Level.Day,"0",2);
 Time = PrePad(Level.Hour,"0",2)$":"$PrePad(Level.Minute,"0",2)$":"$PrePad(Level.Second,"0",2);
 return Date$"-"$Time;
}
// NOTE: may cause an infinite loop if p=""
function string PrePad(coerce string s, string p, int i) {
 while (Len(s) < i)
  s = p$s;
 return s;
}
function bool StrStartsWith(string s, string x) {
 return (InStr(s,x) == 0);
 // return (Left(s,Len(x)) ~= x);
}
function bool StrEndsWith(string s, string x) {
 return (Right(s,Len(x)) ~= x);
}
function bool StrContains(String s, String x) {
 return (InStr(s,x) > -1);
}
function String StrAfter(String s, String x) {
 return StrAfterFirst(s,x);
}
function String StrAfterFirst(String s, String x) {
 return Mid(s,Instr(s,x)+Len(x));
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
 return StrBeforeFirst(s,x);
}
function string StrBeforeFirst(string s, string x) {
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
// Converts a string to lower-case.
function String Locs(String in) {
 local String out;
 local int i;
 local int c;
 out = "";
 for (i=0;i<Len(in);i++) {
  c = Asc(Mid(in,i,1));
  if (c>=65 && c<=90) {
   c = c + 32;
  }
  out = out $ Chr(c);
 }
 return out;
}
// Will get all numbers from string.
// If breakAtFirst is set, will get first number, and place the remainder of the string in rest.
// Will accept all '.'s only leading '-'s
function String StrFilterNum(String in, optional bool breakAtFirst, optional out String rest) {
 local String out;
 local int i;
 local int c;
 local bool onNum;
 out = "";
 onNum = false;
 for (i=0;i<Len(in);i++) {
  c = Asc(Mid(in,i,1));
  if ( (c>=Asc("0") && c<=Asc("9")) || c==Asc(".") || (c==Asc("-") && !onNum) ) {
   out = out $ Chr(c);
   onNum = true;
  } else {
   if (onNum && breakAtFirst) {
    // onNum = false;
    // out = out $ " ";
    rest = Mid(in,i);
    return out;
   }
  }
 }
 rest = "";
 return out;
}
// UT2k4 had Repl(in,search,replace).
function String StrReplace(String in, String search, String replace) {
 return StrReplaceAll(in,search,replace);
}
function String StrReplaceAll(String in, String search, String replace) {
 local String out;
 local int i;
 out = "";
 for (i=0;i<Len(in);i++) {
  if (Mid(in,i,Len(search)) == search) {
   in = Left(in,i) $ replace $ Mid(in,i+Len(search));
   i = i - Len(search) + Len(replace);
  } else {
   out = out $ Mid(in,i,1);
  }
 }
 return out;
}
function String StrPop(out String str, String delimiter) {
 local int i;
 local String result;
 i = InStr(str,delimiter);
 if (i>=0) {
  result = Left(str,i);
  str = Mid(str,i+Len(delimiter));
 } else {
  result = str;
  str = "";
 }
 return result;
}
