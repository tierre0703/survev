// Port of the shared PIXI surface used by renderer.ts / map.ts / objects/*.
//
// The web client draws everything with a small subset of PIXI:
//   Graphics (beginFill/moveTo/lineTo/drawRect/beginHole), Sprite, Container,
//   tint/alpha/blend, stencil masks and manual z-ordering (__zOrd/__zIdx).
//
// Unity has no drop-in equivalent, so this adapter exposes exactly those calls
// and forwards them to Unity meshes/sprites. Porting the game draw code against
// this interface keeps it diffable against the original TypeScript.
//
// IMPORTANT: keep the geometry semantics identical (y-down, fill before stroke),
// and mirror the z-sort contract from renderer.ts (`container.__zOrd`, `__zIdx`).
namespace Survev.Render.PixiLike
{
    /// <summary>Minimal PIXI Surface forwarding to Unity (see Assets/Scripts/Render).</summary>
    public interface IPixiSurface
    {
        void BeginFill(uint color, float alpha);
        void EndFill();
        void MoveTo(float x, float y);
        void LineTo(float x, float y);
        void DrawRect(float x, float y, float width, float height);
        void DrawCircle(float x, float y, float radius);
        void Clear();
    }

    /// <summary>Z-order key mirroring PIXI's manual sort fields.</summary>
    public struct ZOrder
    {
        public int Ord;
        public int Idx;
    }
}