import pandas as pd

df = pd.read_csv("read.csv")

df["MBps"] *= 1.048576

df.to_csv("read_fixed.csv", index=False)