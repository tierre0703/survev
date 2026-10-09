// Entry point for the Unity Android client.
//
// The web client has no single "AppDelegate"; this mirrors client/src/main.ts:
// load config + localization, build the menu, then switch to the game scene on
// join. Both scenes are additive so the menu can be re-shown on death/quit.
using UnityEngine;
using UnityEngine.SceneManagement;

namespace Survev.App
{
    /// <summary>Boots the app and hands control to the menu scene.</summary>
    public sealed class Bootstrap : MonoBehaviour
    {
        [SerializeField] private string _menuSceneName = "Menu";

        private void Awake()
        {
            // Landscape-only, matching the web client's landscape layout and the
            // fixed 1280x720 design space used by the UI (see plan.md D5).
            Screen.orientation = ScreenOrientation.LandscapeLeft;
            Screen.sleepTimeout = SleepTimeout.NeverSleep;
            Application.targetFrameRate = 60;
        }

        private void Start()
        {
            SceneManager.LoadScene(_menuSceneName, LoadSceneMode.Additive);
            SceneManager.SetActiveScene(SceneManager.GetSceneByName(_menuSceneName));
        }
    }
}