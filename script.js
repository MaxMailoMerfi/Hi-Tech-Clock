// Перевіряємо, чи є збережені дані
window.onload = function() {
  const username = localStorage.getItem("username");
  const password = localStorage.getItem("password");

  if (username && password) {
    document.getElementById("saved-username").textContent = username;
    document.getElementById("saved-password").textContent = password;
    document.getElementById("form-container").style.display = "none";
    document.getElementById("show-data").style.display = "block";
  }
};

function saveData() {
  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;

  if (username && password) {
    localStorage.setItem("username", username);
    localStorage.setItem("password", password);
    document.getElementById("status").textContent = "✅ Дані збережені!";
    setTimeout(() => location.reload(), 1000);
  } else {
    document.getElementById("status").textContent = "⚠️ Заповніть усі поля!";
  }
}

function clearData() {
  localStorage.removeItem("username");
  localStorage.removeItem("password");
  location.reload();
}
