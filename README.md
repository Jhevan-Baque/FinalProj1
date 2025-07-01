# FinalProj1

# Tumbang Preso 2.0
--
## Features

- **3 Players, 3 Rounds:** Each round, one player is the taya (tagger), the others try to knock down the can and retrieve their slippers.
- **Manual Input:** All player actions are entered by the user.
- **Randomized Outcomes:** Whether the can is knocked down or a tag is successful is determined randomly.
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

## Code Structure

- **tumbang_preso.cpp:**  
  Contains the main game loop, player input handling, and game logic.

- **Parallelism:**  
  - `simulate_can_knockdown()` and `simulate_tag_success()` use threads and futures to simulate random events in parallel, demonstrating safe use of C++ concurrency for non-interactive logic.

- **User Input:**  
  All prompts and input are handled in the main thread to ensure clean, ordered interaction.

---
