# Tumbang Preso 2.0

---

## Features

- **3 Players, 3 Rounds:** Each round, one player is the taya (tagger), the others try to knock down the can and retrieve their slippers.
- **Manual Input:** All player actions are entered by the user.
- **Randomized Outcomes:** Whether the can is knocked down or a tag is successful is determined randomly.
- **Parallelism Demonstration:** Uses C++ threads and futures to simulate random events (can knockdown, tag success), but all user input/output is handled in the main thread for robust interaction.
- **Classic Mechanics:** Players who throw must retrieve their slippers. If all slippers are on the field, the taya can "bait" the can, giving everyone a chance to retrieve.

---

## How to Play

1. **Start the Game:**  
   The game welcomes you and randomly selects the first taya (tagger).

2. **Throw Phase:**  
   Each non-taya player who has their slipper is prompted to either:
   - `throw` their slipper at the can, or
   - `wait` and skip their turn.

3. **Can Knockdown:**  
   If a player throws, a random check determines if the can is knocked down.

4. **Retrieve Phase:**  
   If the can is knocked down, all players who threw are prompted to either:
   - `run` to retrieve their slipper, or
   - `hide` and wait for a safer opportunity.

   The taya can attempt to tag one of the runners. If tagged, that player becomes the new taya.

5. **Bait Mechanic:**  
   If all slippers are on the field (all non-taya players have thrown), the taya can choose to "bait" the can, allowing all to attempt retrieval.

6. **Rounds:**  
   The game continues for 3 rounds (or until a custom end condition, if you modify the code).

---

---

## Code Features & Functions

| Function Name                | Description                                                                                   |
|------------------------------|-----------------------------------------------------------------------------------------------|
| `get_action_from_user()`     | Prompts a specific player for input, validates the response, and returns the chosen action.   |
| `simulate_can_knockdown()`   | Uses a background thread to randomly determine if a thrown slipper knocks down the can.       |
| `simulate_tag_success()`     | Uses a background thread to randomly determine if the taya successfully tags a runner.        |

### Function Details

- **get_action_from_user(int player_id, const string& prompt, const vector<string>& valid_actions)**
  - Prompts the specified player with a question and a set of valid actions.
  - Loops until the user enters a valid action.
  - Returns the chosen action as a string.

- **simulate_can_knockdown()**
  - Runs a short background thread to simulate the randomness of knocking down the can.
  - Returns `true` if the can is knocked down, `false` otherwise.

- **simulate_tag_success()**
  - Runs a short background thread to simulate the randomness of the taya tagging a runner.
  - Returns `true` if the tag is successful, `false` otherwise.

---
