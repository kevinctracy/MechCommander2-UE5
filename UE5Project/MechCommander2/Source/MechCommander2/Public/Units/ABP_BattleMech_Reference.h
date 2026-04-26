#pragma once
/**
 * ABP_BattleMech_Reference.h
 * Reference document for creating the ABP_BattleMech Animation Blueprint in the UE editor.
 *
 * The Animation Blueprint reads state from AMC2BattleMech and AMC2Mover (the Pawn owner).
 * All variables below must be created in the ABP's Event Graph section "Variables" panel.
 *
 * How to create the ABP:
 *   Content Browser → /Game/Blueprints/Units/ → Add → Animation Blueprint
 *   Parent class: AnimInstance  (or UAnimInstance in C++ terms)
 *   Skeleton: SK_BattleMech_Skeleton  (shared skeleton for all mech types)
 * ──────────────────────────────────────────────────────────────────────────────
 *
 * ── 1. ABP Variables (float/bool, update every frame in Event Blueprint Update Anim) ─
 *
 *   MoveSpeed      float   Get from: PawnOwner → GetVelocity → VectorLength
 *   bIsMoving      bool    MoveSpeed > 50.0
 *   bIsRunning     bool    MoveSpeed > (MaxWalkSpeed * 0.75)  — MaxWalkSpeed from MC2Mover
 *   bIsLimping     bool    Cast to AMC2Mover → bLegsDamaged
 *   bIsJumping     bool    Cast to AMC2BattleMech → bIsJumping
 *   bIsDestroyed   bool    Cast to AMC2Mover → bIsDestroyed
 *   bShutDown      bool    Cast to AMC2Mover → bShutDown
 *   TorsoYawOffset float   Cast to AMC2BattleMech → TorsoYawOffset  (degrees, ±120)
 *
 * ── 2. Animation Asset Assignments ──────────────────────────────────────────
 *
 *   Blend Space BS_MechWalk:
 *     Axis X: MoveSpeed  (range 0–600 UU/s)
 *     Axis Y: (none — 1D blend space)
 *     Point at Speed=0:   A_Mech_Idle
 *     Point at Speed=200: A_Mech_Walk
 *     Point at Speed=600: A_Mech_Run
 *
 *   Blend Space BS_MechWalk_Limping:
 *     Same X axis, but uses A_Mech_Walk_Limp / A_Mech_Run_Limp
 *
 * ── 3. State Machine: SM_MechLocomotion ─────────────────────────────────────
 *
 *   States and transitions:
 *
 *   [Idle] ─────────────────────────────────────────────────────────────────►
 *     Output: A_Mech_Idle
 *     → [Walk/Run]: bIsMoving == true
 *
 *   [Walk/Run] ──────────────────────────────────────────────────────────────
 *     Output: BS_MechWalk driven by MoveSpeed
 *             (if bIsLimping: BS_MechWalk_Limping instead)
 *     → [Idle]: bIsMoving == false
 *     → [Jump]: bIsJumping == true
 *
 *   [Jump] ──────────────────────────────────────────────────────────────────
 *     Output: A_Mech_Jump  (play once, does not loop)
 *     Transition to [Fall] at animation completion (use "Automatic Rule" at sequence end)
 *     → [Fall]: animation reaches end
 *
 *   [Fall] ──────────────────────────────────────────────────────────────────
 *     Output: A_Mech_Fall  (loop — held in air during parabolic arc)
 *     → [GetUp]: bIsJumping == false  (fired by AMC2BattleMech::LandFromJump)
 *
 *   [GetUp] ─────────────────────────────────────────────────────────────────
 *     Output: A_Mech_Land  (play once)
 *     → [Idle]: animation completes (automatic rule)
 *
 *   [Destroyed] ─────────────────────────────────────────────────────────────
 *     Output: A_Mech_Death  (play once, freeze on last frame)
 *     Enter condition: bIsDestroyed == true  (can enter from any state)
 *     No exit transitions.
 *
 *   Note: bShutDown does NOT change the state machine — the pawn stops moving
 *         so MoveSpeed → 0 naturally plays the Idle animation.
 *
 * ── 4. Torso Twist — "Modify Bone" node ────────────────────────────────────
 *
 *   Add a "Modify Bone" node AFTER the state machine output (in the AnimGraph):
 *     State Machine → Modify Bone → Output Pose
 *
 *   Modify Bone settings:
 *     Bone to Modify: "torso"  (must match skeleton bone name exactly)
 *     Rotation Mode:  Add to existing
 *     Rotation Space: Bone Space
 *     Rotation:       (0, 0, TorsoYawOffset)  — Yaw axis only
 *       Wire TorsoYawOffset ABP variable to the Z pin.
 *
 *   This rotates the torso mesh independently of the leg direction, matching
 *   MC2's `SetTorsoAimDirection` / `AimTorsoAt` C++ calls.
 *
 * ── 5. Footstep Notify States ───────────────────────────────────────────────
 *
 *   On A_Mech_Walk and A_Mech_Run sequences:
 *     Add AnimNotify_FootstepDust (UMC2AnimNotify_FootstepDust) at each foot-plant frame:
 *       FootSocket: "foot_l" / "foot_r"
 *       DustScale:  1.0 for walk, 1.5 for run
 *
 *   Frame positions (approximate, tune per specific animation):
 *     Walk:  L-plant ≈ 20%, R-plant ≈ 70%
 *     Run:   L-plant ≈ 15%, R-plant ≈ 65%
 *
 * ── 6. Anim Blueprint Update Event Graph ────────────────────────────────────
 *
 *   "Event Blueprint Update Animation"
 *     ├── IsValid(TryGetPawnOwner) ─┐
 *     │                             ├── GetVelocity → VectorLength → SET MoveSpeed
 *     │                             ├── MoveSpeed > 50  → SET bIsMoving
 *     │                             ├── MoveSpeed > MaxWalkSpeed*0.75 → SET bIsRunning
 *     │                             ├── Cast to AMC2Mover
 *     │                             │     ├── bLegsDamaged → SET bIsLimping
 *     │                             │     ├── bIsDestroyed → SET bIsDestroyed
 *     │                             │     └── bShutDown   → SET bShutDown
 *     │                             └── Cast to AMC2BattleMech
 *     │                                   ├── bIsJumping     → SET bIsJumping
 *     │                                   └── TorsoYawOffset → SET TorsoYawOffset
 *     └── (invalid: skip, no crash)
 *
 * ── 7. Required Animation Assets (create or import from ase_to_fbx.py output) ──
 *
 *   A_Mech_Idle       — idle loop (torso sway, ~2s loop)
 *   A_Mech_Walk       — walk cycle (foot-plant driven, ~1.2s loop)
 *   A_Mech_Run        — run cycle (~0.8s loop)
 *   A_Mech_Walk_Limp  — walk with leg damage (slower, ~1.6s loop)
 *   A_Mech_Run_Limp   — run with leg damage
 *   A_Mech_Jump       — jump start (play-once, ~0.4s)
 *   A_Mech_Fall       — airborne loop (~0.5s loop)
 *   A_Mech_Land       — landing impact (play-once, ~0.5s)
 *   A_Mech_Death      — destruction fall (play-once, freeze last frame, ~1.5s)
 *
 *   Animation source: Source/Data/TGL/ ASE files → ase_to_fbx.py → FBX import.
 *   The 25 gesture types from mech3d.h map to these 9 ABP animation assets.
 *   Gesture groups: IDLE (→Idle), WALK (→Walk/Run), LIMP (→Limp variants),
 *                   FALL/JUMPUP/JUMPDOWN (→Jump/Fall/Land), DAMAGE1/2 (→Death).
 *
 * ── 8. Notes for non-mech AnimBPs ───────────────────────────────────────────
 *
 *   BP_GroundVehicle uses a simpler AnimBP or no AnimBP at all — turret rotation
 *   is driven by AMC2GroundVehicle::Tick via SetBoneRotationByName("turret").
 *
 *   BP_Turret uses no AnimBP — rotation is driven by AMC2Turret::Tick the same way.
 */
