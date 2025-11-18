# CCSD Workflow

## Computation Flow

\`\`\`
┌─────────────┐
│  Load Config│
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Initialize  │
│     MPI     │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Setup      │
│  Vectors    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   CCSD      │
│  Iteration  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Output    │
│   Results   │
└─────────────┘
\`\`\`

## Parallel Strategy

Each MPI process handles a portion of the T1 and T2 amplitudes.
