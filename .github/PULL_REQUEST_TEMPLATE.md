## Summary
<!-- One or two sentences: what does this PR do and why? -->

## Layers Changed
<!-- Check all that apply -->
- [ ] STM32 firmware (`stm32/`)
- [ ] Linux controller (`linux/`)
- [ ] Web API (`web/api/`)
- [ ] Frontend (`web/frontend/`)
- [ ] Docs / infra only

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / cleanup
- [ ] Safety-critical change
- [ ] Protocol / IPC change (JSON-RPC interface or UDS socket)

## Safety Checklist
<!-- Required for any change touching state_machine, estop, pace control, or motor commands -->
- [ ] Estop behavior is unchanged or intentionally modified and reviewed
- [ ] State machine transitions are correct (idle -> running -> stopped)
- [ ] No path exists that bypasses estop

## IPC / Protocol Changes
<!-- Fill in if the JSON-RPC interface between the Linux controller and API changed -->
- Methods added/changed/removed: <!-- e.g. `set_pace` params updated -->
- `MockRobotControllerClient` updated to match? [ ] Yes [ ] N/A

## Testing
- [ ] Tested with mock client (`USE_MOCK_TELEMETRY=true`)
- [ ] Tested on real hardware
- [ ] STM32 firmware flashed and verified
- [ ] Linux controller built and run (`cmake --build`)
- [ ] API server started cleanly (`run_api.sh`)
- [ ] Frontend UI verified in browser

## Notes for Reviewer
<!-- Anything tricky, known limitations, or follow-up work -->
