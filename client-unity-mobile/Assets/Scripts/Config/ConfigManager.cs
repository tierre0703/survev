// Port of the ConfigManager half of client/src/config.ts.
//
// The web client stores settings in localStorage; Unity uses PlayerPrefs, which
// is SharedPreferences on Android. Keys and defaults must match the web client so
// settings survive a conversion and the UI shows the same values.
using UnityEngine;

namespace Survev.Config
{
    /// <summary>Typed settings store backed by <see cref="PlayerPrefs"/>.</summary>
    public static class ConfigManager
    {
        public const string KeyMasterVolume = "sl-master-volume";
        public const string KeySoundVolume = "sl-sound-volume";
        public const string KeyMusicVolume = "sl-music-volume";
        public const string KeyMute = "sound-mute";
        public const string KeyLanguage = "language";
        public const string KeyPlayerName = "player-name";

        public static float MasterVolume
        {
            get => PlayerPrefs.GetFloat(KeyMasterVolume, 1f);
            set => SetFloat(KeyMasterVolume, value);
        }

        public static float SoundVolume
        {
            get => PlayerPrefs.GetFloat(KeySoundVolume, 1f);
            set => SetFloat(KeySoundVolume, value);
        }

        public static float MusicVolume
        {
            get => PlayerPrefs.GetFloat(KeyMusicVolume, 1f);
            set => SetFloat(KeyMusicVolume, value);
        }

        public static bool Mute
        {
            get => PlayerPrefs.GetInt(KeyMute, 0) != 0;
            set => SetInt(KeyMute, value ? 1 : 0);
        }

        public static string Language
        {
            get => PlayerPrefs.GetString(KeyLanguage, string.Empty);
            set => SetString(KeyLanguage, value);
        }

        public static string PlayerName
        {
            get => PlayerPrefs.GetString(KeyPlayerName, string.Empty);
            set => SetString(KeyPlayerName, value);
        }

        public static void Save()
        {
            PlayerPrefs.Save();
        }

        private static void SetFloat(string key, float value)
        {
            PlayerPrefs.SetFloat(key, value);
            PlayerPrefs.Save();
        }

        private static void SetInt(string key, int value)
        {
            PlayerPrefs.SetInt(key, value);
            PlayerPrefs.Save();
        }

        private static void SetString(string key, string value)
        {
            PlayerPrefs.SetString(key, value);
            PlayerPrefs.Save();
        }
    }
}