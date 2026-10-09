// Port of shared/lib/bitBuffer.ts — the bit view every net packet is built on.
//
// This file is the Unity counterpart of Core/BitBuffer.cs. It is intentionally
// small: the full reader/writer lives in Assets/Scripts/Core/BitBuffer.cs and is
// unit-tested against captured frames from the web client. Keep the two in sync
// and never change one without the other.
//
// See Assets/Scripts/Core/BitBuffer.cs for the implementation and
// Assets/Tests/EditMode/BitBufferTests.cs for the fixtures.
