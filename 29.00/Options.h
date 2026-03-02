#pragma once

class UHeliosConfiguration
{
public:
	static inline bool bIsLateGame = false;
	static inline bool bIsTournament = false;
	static inline bool bIsProd = true;

	static inline int Port = 7777;

	static inline std::string PlaylistID = "/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo";
	static inline std::string Region = "NAE";
};