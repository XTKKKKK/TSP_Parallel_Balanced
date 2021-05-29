let str = "";
console.log(10);
console.log(90);
for (let i = 0; i < 10; i++) {
  for (let j = 0; j < 10; j++) {
    if (i == j) {
      continue;
    } else {
      console.log(i + " " + j + " " + Math.ceil(Math.random() * 10));
    }
  }
}
