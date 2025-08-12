// Примітивний пароль (для тесту)
const correctPassword = "12345";

function checkPassword() {
  const input = document.getElementById("password").value;
  const error = document.getElementById("error");

  if (input === correctPassword) {
    document.getElementById("login").style.display = "none";
    document.getElementById("content").style.display = "block";
  } else {
    error.textContent = "Невірний пароль!";
  }
}
