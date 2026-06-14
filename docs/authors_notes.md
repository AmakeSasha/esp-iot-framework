# Author's notes

Here are the answers that I considered important, at least for me. I think it will be useful for me to read this in the future.

* **Where did the name of this project come from?**

  I just didn't want to waste time overthinking an overly fancy or complicated name. `ESP IoT Framework` describes exactly what this project is and does, so I went with it. It is straightforward, clean, and tells you its purpose right away.

* **What is the purpose of this project? What is its future?**
  
  This project is being created as the main (perhaps the only) part of my portfolio. I develop it when I feel like it. Since I really liked this project myself, I will continue to develop it in the future.

* **Has the project been officially audited, certified, or checked for MISRA C compliance?**

  No official audit or commercial certification has been conducted (I don't have the budget for this). However, the code is checked through the `cppcheck` static analyzer, which ensures that there are no known patterns of undefined behavior in the code.

* **Why do all functions, constants, and macros start with `eif_`? What does `eif` mean?**

  I did this on purpose, following the exact same pattern as ESP-IDF where everything starts with `esp_`. It completely rules out naming conflicts so your code and third-party libraries won't accidentally clash. 

  As for the meaning, `eif` is simply a short prefix for the project's full name: <code><b>E</b>SP <b>I</b>oT <b>F</b>ramework</code>.

* **Why do component names start with the full project name instead of the abbreviation?**

  Because the abbreviation `eif` means absolutely nothing to someone who doesn't know the acronym. While the short `eif_` prefix is great for keeping your code concise, components should clearly show what project they belong to without forcing anyone to guess.