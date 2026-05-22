(() => {
  const size = 48;
  const color = {
    outline: '#182235',
    outlineSoft: '#314158',
    skin: '#ffc493',
    skinShade: '#e79a6a',
    hair: '#5b2b12',
    hairLight: '#9a5420',
    shirt: '#2563eb',
    shirtLight: '#60a5fa',
    scarf: '#f59e0b',
    trousers: '#334155',
    boot: '#7c3f12',
    bootLight: '#b45309',
  };

  function make(draw) {
    const canvas = document.createElement('canvas');
    canvas.width = size;
    canvas.height = size;
    const ctx = canvas.getContext('2d');
    ctx.imageSmoothingEnabled = false;
    draw(ctx);
    return canvas;
  }

  function block(ctx, fill, x, y, w, h) {
    ctx.fillStyle = fill;
    ctx.fillRect(x, y, w, h);
  }

  function drawShadow(ctx) {
    block(ctx, 'rgba(2, 6, 23, 0.28)', 11, 42, 26, 4);
    block(ctx, 'rgba(15, 23, 42, 0.14)', 15, 40, 18, 3);
  }

  function drawLegs(ctx, step) {
    const leftY = step > 0 ? 32 : 31;
    const rightY = step < 0 ? 32 : 31;
    block(ctx, color.outline, 15 + step, 30, 9, 14);
    block(ctx, color.outline, 24 - step, 30, 9, 14);
    block(ctx, color.trousers, 17 + step, leftY, 5, 8);
    block(ctx, color.trousers, 26 - step, rightY, 5, 8);
    block(ctx, color.boot, 15 + step, 39 + (step > 0 ? 1 : 0), 9, 5 - (step > 0 ? 1 : 0));
    block(ctx, color.boot, 24 - step, 39 + (step < 0 ? 1 : 0), 9, 5 - (step < 0 ? 1 : 0));
    block(ctx, color.bootLight, 17 + step, 40 + (step > 0 ? 1 : 0), 4, 2);
    block(ctx, color.bootLight, 26 - step, 40 + (step < 0 ? 1 : 0), 4, 2);
  }

  function drawFront(step) {
    return make(ctx => {
      drawShadow(ctx);
      drawLegs(ctx, step);

      block(ctx, color.outline, 12, 16, 24, 19);
      block(ctx, color.shirt, 14, 18, 20, 14);
      block(ctx, color.shirtLight, 16, 19, 6, 9);
      block(ctx, color.outlineSoft, 14, 31, 20, 3);
      block(ctx, color.scarf, 19, 18, 10, 3);
      block(ctx, color.scarf, 27, 20, 3, 5);

      block(ctx, color.outline, 7 - step, 19, 7, 13);
      block(ctx, color.outline, 34 + step, 19, 7, 13);
      block(ctx, color.shirt, 9 - step, 20, 4, 7);
      block(ctx, color.shirt, 35 + step, 20, 4, 7);
      block(ctx, color.skinShade, 8 - step, 27, 5, 5);
      block(ctx, color.skin, 35 + step, 27, 5, 5);

      block(ctx, color.outline, 13, 3, 22, 18);
      block(ctx, color.skinShade, 16, 8, 16, 12);
      block(ctx, color.skin, 17, 7, 14, 11);
      block(ctx, color.hair, 13, 3, 22, 7);
      block(ctx, color.hair, 13, 8, 4, 5);
      block(ctx, color.hair, 31, 8, 4, 5);
      block(ctx, color.hairLight, 18, 4, 9, 2);
      block(ctx, color.outline, 19, 12, 3, 3);
      block(ctx, color.outline, 27, 12, 3, 3);
      block(ctx, '#eff6ff', 20, 12, 1, 1);
      block(ctx, '#eff6ff', 28, 12, 1, 1);
      block(ctx, color.skinShade, 23, 16, 4, 1);
    });
  }

  function drawBack(step) {
    return make(ctx => {
      drawShadow(ctx);
      drawLegs(ctx, -step);

      block(ctx, color.outline, 11, 15, 26, 20);
      block(ctx, color.shirt, 13, 17, 22, 16);
      block(ctx, color.shirtLight, 15, 18, 5, 11);
      block(ctx, color.outlineSoft, 14, 31, 20, 3);
      block(ctx, color.scarf, 18, 17, 12, 3);

      block(ctx, color.outline, 7 + step, 19, 7, 13);
      block(ctx, color.outline, 34 - step, 19, 7, 13);
      block(ctx, color.shirt, 9 + step, 20, 4, 7);
      block(ctx, color.shirt, 35 - step, 20, 4, 7);
      block(ctx, color.skinShade, 8 + step, 27, 5, 5);
      block(ctx, color.skinShade, 35 - step, 27, 5, 5);

      block(ctx, color.outline, 13, 3, 22, 18);
      block(ctx, color.hair, 15, 5, 18, 14);
      block(ctx, color.hairLight, 18, 6, 10, 3);
      block(ctx, color.hair, 13, 8, 4, 8);
      block(ctx, color.hair, 31, 8, 4, 8);
    });
  }

  function drawRight(step) {
    return make(ctx => {
      drawShadow(ctx);
      block(ctx, color.outline, 14, 30, 9, 14);
      block(ctx, color.outline, 25, 30, 9, 14);
      block(ctx, color.trousers, 16, 31 + (step > 0 ? 1 : 0), 5, 8);
      block(ctx, color.trousers, 27, 31 + (step < 0 ? 1 : 0), 5, 8);
      block(ctx, color.boot, 13, 39 + (step > 0 ? 1 : 0), 11, 5 - (step > 0 ? 1 : 0));
      block(ctx, color.boot, 25, 39 + (step < 0 ? 1 : 0), 11, 5 - (step < 0 ? 1 : 0));

      block(ctx, color.outline, 12, 16, 24, 19);
      block(ctx, color.shirt, 15, 18, 18, 14);
      block(ctx, color.shirtLight, 22, 19, 6, 9);
      block(ctx, color.outlineSoft, 15, 31, 19, 3);
      block(ctx, color.scarf, 20, 18, 10, 3);
      block(ctx, color.scarf, 28, 20, 3, 5);

      block(ctx, color.outline, 25 - step, 19, 8, 13);
      block(ctx, color.shirt, 27 - step, 20, 4, 7);
      block(ctx, color.skin, 28 - step, 27, 5, 5);
      block(ctx, color.outline, 10 + step, 20, 6, 12);
      block(ctx, color.skinShade, 11 + step, 27, 4, 4);

      block(ctx, color.outline, 16, 3, 20, 18);
      block(ctx, color.skinShade, 21, 8, 13, 12);
      block(ctx, color.skin, 22, 7, 11, 11);
      block(ctx, color.hair, 16, 3, 19, 8);
      block(ctx, color.hair, 16, 8, 5, 8);
      block(ctx, color.hairLight, 21, 4, 8, 2);
      block(ctx, color.outline, 29, 12, 3, 3);
      block(ctx, '#eff6ff', 30, 12, 1, 1);
    });
  }

  function mirror(canvas) {
    return make(ctx => {
      ctx.translate(size, 0);
      ctx.scale(-1, 1);
      ctx.drawImage(canvas, 0, 0);
    });
  }

  const right = [drawRight(0), drawRight(-2), drawRight(2)];
  const frames = {
    down: [drawFront(0), drawFront(-2), drawFront(2)],
    up: [drawBack(0), drawBack(-2), drawBack(2)],
    right,
    left: right.map(mirror),
  };

  window.SPRITES = window.SPRITES || {};
  window.SPRITES.player = frames.down[0];
  window.SPRITES.playerWalk = frames;
})();
