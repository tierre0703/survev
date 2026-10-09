// Minimal UI theme: the web client's colours/sizes from css/app.css + game.css.
//
// Every value here is copied from the web stylesheets so the Android UI matches
// 1:1. When in doubt, grep the CSS: colors are CSS hex, sizes are CSS px in the
// 1280x720 design space (see plan.md D2).
using UnityEngine;

namespace Survev.UI
{
    /// <summary>Named colours and sizes ported from the web CSS.</summary>
    public static class UiTheme
    {
        // css/app.css: html, body { background: #80af49; }
        public static readonly Color PageBackground = Hex("#80af49");

        // css/app.css: .menu-block { background: rgba(0,0,0,.5); }
        public static readonly Color MenuBlock = new Color(0f, 0f, 0f, 0.5f);

        // css/app.css: a:link { color: #7cfc00; }
        public static readonly Color Link = Hex("#7cfc00");
        public static readonly Color LinkHover = Hex("#51a500");

        // .btn-green body + shadow.
        public static readonly Color BtnGreen = Hex("#83af50");
        public static readonly Color BtnGreenShadow = Hex("#5b7a38");

        // .btn-darken body + shadow.
        public static readonly Color BtnDarken = Hex("#7a7a7a");
        public static readonly Color BtnDarkenShadow = Hex("#3e3e3e");

        // .btn-hollow border / selected border.
        public static readonly Color BtnHollow = Color.white;
        public static readonly Color BtnHollowSelected = Hex("#00ff00");

        // Gold news headers.
        public static readonly Color NewsHeader = Hex("#ffd700");

        /// <summary>Side padding from :root { --side-pad: 12px; }</summary>
        public const float SidePad = 12f;

        /// <summary>The UI design resolution (matches the web canvas layout).</summary>
        public const float DesignWidth = 1280f;
        public const float DesignHeight = 720f;

        /// <summary>Parse a CSS hex colour ("#rgb", "#rrggbb", "#rrggbbaa").</summary>
        public static Color Hex(string css)
        {
            if (string.IsNullOrEmpty(css) || css[0] != '#')
            {
                return Color.white;
            }
            string s = css.Substring(1);
            if (s.Length == 3)
            {
                int r = ConvertNibble(s[0]);
                int g = ConvertNibble(s[1]);
                int b = ConvertNibble(s[2]);
                return new Color((r * 17) / 255f, (g * 17) / 255f, (b * 17) / 255f, 1f);
            }
            if (s.Length != 6 && s.Length != 8)
            {
                return Color.white;
            }
            float r6 = System.Convert.ToInt32(s.Substring(0, 2), 16) / 255f;
            float g6 = System.Convert.ToInt32(s.Substring(2, 2), 16) / 255f;
            float b6 = System.Convert.ToInt32(s.Substring(4, 2), 16) / 255f;
            float a6 = s.Length == 8 ? System.Convert.ToInt32(s.Substring(6, 2), 16) / 255f : 1f;
            return new Color(r6, g6, b6, a6);
        }

        private static int ConvertNibble(char c)
        {
            int value = System.Convert.ToInt32(c.ToString(), 16);
            return value;
        }
    }
}