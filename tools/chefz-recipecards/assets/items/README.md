# Item images

Drop cut-out item pictures here and name them in `../../item-images.json`:

```json
{ "ChefZ_Cheese": "assets/items/cheese.png", "cat:SALT": "assets/items/salt.png" }
```

**What works best:** square PNG with an alpha channel, 128 px or more, the item
filling most of the frame. The card scales it up beyond the cell on purpose, so
a picture with wide empty margins ends up looking small.

**Keys, not filenames, are what matter.** `ChefZ_Corn` is a concrete class,
`cat:ROOT_VEGETABLE` a category slot, `tag:CHEFZ_HERB` a tag slot. Run the
generator once and it lists every key it wanted, with how often each is used —
that list is the shopping list.

A real picture always beats the vector glyph the generator falls back to.
Nothing else has to change: enter it here and it appears on the next run.
