# Session handoffs

Create one file per active work session when work cannot be completed in a single session:

```text
YYYY-MM-DD-RLS-NN.md
```

Use this structure:

```markdown
# RLS-NN handoff — YYYY-MM-DD

- Status: In flight | Blocked
- Done: …
- Evidence: …
- Blocked by: …
- Next: …
- Do not: …
```

Do not create empty handoffs. Durable decisions belong in `../decisions.md`; acceptance evidence
belongs in the relevant story.
