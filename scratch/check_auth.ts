
const usernameInput = "admin".toString().trim().toLowerCase();
const passwordInput = "admin1234".toString().trim();

console.log(`Username: "${usernameInput}"`);
console.log(`Password: "${passwordInput}"`);

if (usernameInput === "admin" && passwordInput === "admin1234") {
    console.log("MATCH!");
} else {
    console.log("FAIL!");
}
