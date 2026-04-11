Privacy Policy: Team Fortress 2 Vintage (TF2V)
This application uses the Discord Game SDK (Rich Presence) to display your in-game activity on your Discord profile.

1. Data Processing
To provide Rich Presence, TF2V processes the following information locally on your computer:

Unique Identifiers: Your SteamID and Discord User ID.

Gameplay State: Your current map, team, and character class.

Network Information: The IP address and port of the server you are currently playing on (including SourceTV ports).

Game Events: Real-time events such as team changes, death/respawn status, and tournament match states.

2. Data Usage & Sharing
Purpose: This data is used solely to populate the "Rich Presence" display (e.g., "Playing as RED Soldier on cp_badlands") and to enable the "Join" and "Spectate" buttons for your friends.

Third Parties: All data is sent directly from your game client to Discord Inc. TF2V does not host any external databases, and your data is never sent to, stored by, or seen by the project developers.

Retention: TF2V does not retain any of your data. The information is volatile and only exists while the game client is running.

3. User Control (Opt-Out)
You can disable this integration at any time by:

Disabling "Activity Status" within your Discord User Settings.

Setting cl_discord_presence_enabled to 0 in your TF2V configuration file (tf2vintage/cfg/config.cfg).

Removing the TF2V binary folder (tf2vintage/bin/) from your game directory.