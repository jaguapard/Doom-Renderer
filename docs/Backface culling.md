# Backface culling

**The project has backface culling implemented and toggleable by a key. Why is it disabled by default?**
Backface culling does cause rendering glitches if the triangles being viewed from the backside is considered normal. It also interferes with transparency. The performance gain (~5-20% depending on various factors) does not seem to be worth it. It is a very useful way of ensuring proper direction for normal vectors though, that's why it stays in.