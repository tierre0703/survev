// Build entry point invoked by the Unity Editor in batch mode.
//
// Usage (adjust the Editor path to your installed 2020.3.x):
//   & "C:\Program Files\Unity\Hub\Editor\2020.3.48f1\Editor\Unity.exe" ^
//       -batchmode -quit -projectPath . ^
//       -executeMethod Survev.EditorTools.BuildScript.BuildAndroid -logFile build.log
using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Survev.EditorTools
{
    /// <summary>Command-line Android build (APK by default, AAB optional).</summary>
    public static class BuildScript
    {
        private static readonly string[] Scenes =
        {
            "Assets/Scenes/Bootstrap.unity",
            "Assets/Scenes/Menu.unity",
            "Assets/Scenes/Game.unity",
        };

        public static void BuildAndroid()
        {
            string output = Path.Combine("Builds", "SurvevMobile.apk");
            Directory.CreateDirectory("Builds");

            var options = new BuildPlayerOptions
            {
                scenes = Scenes,
                locationPathName = output,
                target = BuildTarget.Android,
                options = BuildOptions.None,
            };

            // Landscape-only, matching the web client layout (plan.md D5).
            PlayerSettings.defaultInterfaceOrientation = UIOrientation.LandscapeLeft;
            PlayerSettings.SetApplicationIdentifier(
                BuildTargetGroup.Android, "com.survev.mobile");
            // The client must run on both 32-bit and 64-bit ARM devices:
            //   ARMv7  = armeabi-v7a
            //   ARM64  = arm64-v8a
            // (x86/x86_64 are intentionally omitted — the web/axmol clients do the
            // same; the x86_64 emulator can run the ARM build only under translation.)
            PlayerSettings.Android.targetArchitectures =
                AndroidArchitecture.ARMv7 | AndroidArchitecture.ARM64;
            // Mono for readable dev stack traces; IL2CPP for release is selected
            // via the build menu / CI flag. Keep the repo default at Mono so the
            // batch build stays fast unless overridden.
            PlayerSettings.SetScriptingBackend(BuildTargetGroup.Android, ScriptingImplementation.Mono2x);
            PlayerSettings.SetApiCompatibilityLevel(
                BuildTargetGroup.Android, ApiCompatibilityLevel.NET_Standard_2_0);
            PlayerSettings.Android.minSdkVersion = AndroidSdkVersions.AndroidApiLevel24;
            PlayerSettings.Android.bundleVersionCode = 1;
            PlayerSettings.bundleVersion = "0.4.3";

            BuildReport report = BuildPipeline.BuildPlayer(options);
            if (report.summary.result != BuildResult.Succeeded)
            {
                throw new Exception($"Build failed: {report.summary.result}");
            }
            Debug.Log($"Build succeeded: {output} ({report.summary.totalSize} bytes)");
        }
    }
}