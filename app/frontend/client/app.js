(() => {
  const status = document.getElementById('status');
  if (status) {
    status.textContent = '干净客户端占位：只允许接入 /api/client/* 真实协议。';
  }
})();
