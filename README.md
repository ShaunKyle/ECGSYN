# ecgsyn.c

> A software package for generating realistic ECG waveforms.

C implementation of ECGSYN, with complete source code and small modifications.

## Usage

CLI program.

```text
ECGSYN: A program for generating a realistic synthetic ECG

Copyright (c) 2003 by Patrick McSharry & Gari Clifford. All rights reserved.

Flags:
-O Name of output data file
-n Approximate number of heart beats
-s ECG sampling frequency [Hz]
-S Internal Sampling frequency [Hz]
-a Amplitude of additive uniform noise [mV]
-h Heart rate mean [bpm]
-H Heart rate standard deviation [bpm]
-f Low frequency [Hz]
-F High frequency [Hz]
-v Low frequency standard deviation [Hz]
-V High frequency standard deviation [Hz]
-q LF/HF ratio
-R Random number generator seed
```

Output files

`ecgsyn.dat` contains synthetic ECG waveform. Each entry has three 
space-delimited items: time (s), voltage (V), and PQRST peak label. 
Name of file can be changed with `-O` flag.

`rr.dat`

`rrpc.dat`

## Background

ECGSYN is a collection of software packages for generating realistic ECG 
waveforms, based on the 2003 paper "A dynamical model for generating synthetic 
electrocardiogram signals".

The C implementation of ECGSYN is licensed under GPLv2, but depends on two 
routines from "Numerical Recipes in C": `ran1` and `dfour1`. The code for 
Numerical Recipes is only available under a restrictive single-user or 
institutional license, meaning that these routines are not provided with the 
source code of ECGSYN. However, the textbook chapters describing the theory 
behind their implementation are freely available.

## List of modifications from original ECGSYN

Implementations for `ran1` and `dfour1` provided.

Cross-platform makefile (tested on...).

TODO: Modern C standard, address compiler warnings.

TODO: Improve CLI

TODO: Library instead of CLI?

## License

GNU GPLv3, as per terms of original ECGSYN license.

## Bibliography

[McSharry PE, Clifford GD, Tarassenko L, Smith L. A dynamical model for generating synthetic electrocardiogram signals. IEEE Transactions on Biomedical Engineering 50(3): 289-294; March 2003.](http://web.mit.edu/~gari/www/papers/ieeetbe50p289.pdf)
- Original paper describing dynamical model.
- Numerical integration (4th order Runge-Kutta) of three coupled ODEs.
- HTML version of paper on [physionet archive](https://archive.physionet.org/physiotools/ecgsyn/paper/)

[ECGSYN - A realistic ECG waveform generator](https://physionet.org/content/ecgsyn/1.0.0/)
- A collection of software packages for generating realistic ECG waveforms, 
based on the method in "A dynamical model for generating synthetic 
electrocardiogram signals".
- Additional feature not described in the original paper: user can modify the 
morphology of the P-QRS-T cycle.
- Implementations available in MATLAB (original?), C, and Java.

[Numerical Recipes](https://numerical.recipes/)
- A series of books on scientific computing.
- Book contents available online for free, source code available to purchase.

[ecgsyn-arduino](https://github.com/flandrade/ecgsyn-arduino)
- Port of ECGSYN to the Arduino platform.
- Source code includes implementations of `ran1` and `dfour1`.
