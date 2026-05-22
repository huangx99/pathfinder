(() => {
  const c = document.createElement('canvas');
  c.width = 48; c.height = 48;
  const x = c.getContext('2d');

  // Shadow
  x.fillStyle = 'rgba(0,0,0,0.25)';
  x.fillRect(12, 42, 24, 4);

  // Legs (dark pants)
  x.fillStyle = '#1e293b';
  x.fillRect(17, 30, 6, 14);  // left leg
  x.fillRect(25, 30, 6, 14);  // right leg

  // Boots
  x.fillStyle = '#451a03';
  x.fillRect(16, 40, 8, 4);
  x.fillRect(24, 40, 8, 4);

  // Torso (blue tunic)
  x.fillStyle = '#1d4ed8';
  x.fillRect(14, 18, 20, 14);
  x.fillStyle = '#3b82f6';  // highlight
  x.fillRect(16, 20, 6, 10);

  // Belt
  x.fillStyle = '#78350f';
  x.fillRect(14, 30, 20, 3);
  x.fillStyle = '#b45309';  // buckle
  x.fillRect(22, 30, 4, 3);

  // Arms
  x.fillStyle = '#fca5a5';  // skin
  x.fillRect(10, 20, 4, 10);   // left arm
  x.fillRect(34, 20, 4, 10);   // right arm

  // Hands
  x.fillRect(9, 28, 5, 5);
  x.fillRect(34, 28, 5, 5);

  // Head (skin)
  x.fillStyle = '#fca5a5';
  x.fillRect(16, 6, 16, 14);

  // Hair
  x.fillStyle = '#451a03';
  x.fillRect(14, 4, 20, 6);
  x.fillRect(14, 6, 4, 6);
  x.fillRect(30, 6, 4, 6);

  // Eyes
  x.fillStyle = '#1e293b';
  x.fillRect(20, 12, 3, 3);
  x.fillRect(27, 12, 3, 3);
  x.fillStyle = '#ffffff';
  x.fillRect(21, 13, 1, 1);
  x.fillRect(28, 13, 1, 1);

  // Mouth
  x.fillStyle = '#be123c';
  x.fillRect(22, 17, 4, 1);

  // Backpack (behind torso, drawn on top at sides)
  x.fillStyle = '#713f12';
  x.fillRect(12, 16, 3, 14);
  x.fillRect(33, 16, 3, 14);

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.player = c;
})();
