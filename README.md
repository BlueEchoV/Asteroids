**Welcome to Asteroids!!!**

Video Demo: https://youtu.be/PDNSYJxtyQI

**Game Description**

A from-scratch implementation of the classic arcade game Asteroids, built in C++ using SDL2 for rendering, input, and audio handling (audio not yet implemented). The player controls a Tie Fighter-inspired ship that can rotate, thrust, and fire projectiles in a wrap-around (toroidal) playfield filled with destructible asteroids.
Key Features

- Smooth ship movement with thrust and rotation
- Asteroids that spin, move independently, and break into smaller fragments when destroyed
- Screen wrapping for both ship and asteroids
- Bullet management with limited lifetime
- Lives system with visual indicators (extra ships displayed in corner)
- Brief immunity period after respawn
- Safe spawn zones (asteroids avoid spawning near player at start)
- Main menu with high score display
- Game over screen with name entry for high score submission
- Persistent high scores saved to file
- Custom bitmap font rendering
- Multiple asteroid sizes/models with increasing difficulty as they break

**Technical Goals**

The project was created as a learning exercise to deepen understanding of low-level game development in C++. Primary objectives included:

- Implementing core 2D game mechanics (movement, rotation, collision, wrapping) without an engine
- Building a simple but robust sprite and entity system
- Handling circular collision detection efficiently in a toroidal space
- Loading and rendering PNG assets with transparency using stb_image
- Creating a custom text rendering system from a bitmap font
- Managing game states (menu, gameplay, game over)
- Implementing file I/O for persistent high scores
- Practicing clean, readable C++ code with manual resource management
