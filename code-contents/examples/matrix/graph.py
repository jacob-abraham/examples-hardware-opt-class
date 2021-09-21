
import re
from typing import Tuple
import scipy as sp
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy import stats as sst
import stat as st
import os

def cleanData(data, iterations=1):
    for _ in range(iterations):
        low_per, high_per = np.percentile(data, [2.5, 97.5])
        data[(data < low_per) | (data > high_per)] = np.nan
    return data

def cleanNAN(data):
    clean = data[~np.isnan(data)]
    return clean

def load_file(filename):
    lines = open(filename, "r").readlines()
    lines.reverse() # reverse array for more efficentr pop from front
    data_entries = []
    num_regex = re.compile(r"(\d+\.\d+)([a-zA-Z]{0,2})")
    # parse whole file
    while lines:
        # if we find a line with a title, pop and process
        l = lines.pop()
        if m := re.match(r"(.+):\s*Mean:", l):
            title = m[1]
            data = []
            units = []
            # need to get all the following lines with data
            while lines:
                l = lines.pop()
                # if line starts with data, get it all
                if re.match(num_regex, l):
                    for m in re.finditer(num_regex, l):
                        data.append(float(m[1]))
                        units.append(m[2])
                else: # out of data, next entry
                    lines.append(l)
                    break
            # if all units dont match, fail
            if units and not all([x == units[0] for x in units]):
                data = []
                units = []
            data_entries.append({"title": title, "data": np.array(data), "unit": units[0] if units else ""})
    return data_entries

def statStringForData(data, unit=""):
    return f"Mean: {np.mean(data):8.4f}{unit} Variance: {np.var(data):8.4f}{unit}^2"

def scatterData(entries, title=""):
    fig, axs = plt.subplots(1, len(entries), figsize=[10, 6.4])
    fig.suptitle(title)
    for idx, entry in enumerate(entries):
        d = entry["data"]

        axs[idx].scatter(d, d, color='black', label="original data")
        d = cleanData(d, 40)
        d = cleanNAN(d)
        axs[idx].scatter(d, d, color='green', label="cleaned outliers")
        axs[idx].set_title(f"{entry['title']} ({entry['unit']})\n{statStringForData(d, entry['unit'])}")
        axs[idx].legend()

        entries[idx]["data"] = d
    
    if len(entries) == 2:
        tval, pval = sst.ttest_ind(entries[0]["data"], entries[1]["data"], equal_var=False)
        fig.text(0.5, 0.05, f"t: {tval}, p: {pval}", horizontalalignment='center', verticalalignment='center', bbox=dict(facecolor='red', alpha=0.5, pad= 3))

def histData(entries, title=""):
    fig, axs = plt.subplots(1, len(entries), figsize=[10, 6.4])
    fig.suptitle(title)
    for idx, entry in enumerate(entries):
        d = entry["data"]

        axs[idx].hist(d, bins=range(9600, 10200, 100))
        axs[idx].legend()

def fitToDist(entry):
    # convert to ns and then use ints
    d = entry["data"]
    d = d*1000
    d = d.astype(np.int)
    list_of_dists = ['alpha','anglit','arcsine','beta','betaprime','bradford','burr','burr12','cauchy','chi','chi2','cosine','dgamma','dweibull','erlang','expon','exponnorm','exponweib','exponpow','f','fatiguelife','fisk','foldcauchy','foldnorm','invweibull','genlogistic','genpareto','gennorm','genexpon','genextreme','gausshyper','gamma','gengamma','genhalflogistic','gilbrat','gompertz','gumbel_r','gumbel_l','halfcauchy','halflogistic','halfnorm','halfgennorm','hypsecant','invgamma','invgauss','invweibull','johnsonsb','johnsonsu','kstwobign','laplace','levy','levy_l','logistic','loggamma','loglaplace','lognorm','lomax','maxwell','mielke','nakagami','ncx2','ncf','nct','norm','pareto','pearson3','powerlaw','powerlognorm','powernorm','rdist','reciprocal','rayleigh','rice','recipinvgauss','semicircular','t','triang','truncexpon','truncnorm','tukeylambda','uniform','vonmises','vonmises_line','wald','weibull_min','weibull_max']

    results = []
    for i in list_of_dists:
        dist = getattr(sst, i)
        param = dist.fit(d)
        a = sst.kstest(d, i, args=param)
        results.append((i,a[0],a[1]))
        
    # H0: dist are identical

        
    results.sort(key=lambda x:float(x[2]), reverse=True)
    for j in results:
        print("{}: statistic={}, pvalue={}".format(j[0], j[1], j[2]))

def plotPDF(data):
    params = sst.johnsonsu.fit(entries[0]["data"])
    params2 = sst.johnsonsu.fit(entries[0]["data"])
    x = np.linspace(sst.johnsonsu.ppf(0.01, *params),
                sst.johnsonsu.ppf(0.99, *params), 100)
    plt.plot(x, sst.johnsonsu.pdf(x, *params),'r-', lw=5, alpha=0.6, label='johnsonsu pdf')

def process(filename):
    entries = load_file(filename)
    name, ext = os.path.splitext(filename)
    #fitToDist(entries[0])

    plt.show()
    #scatterData(entries, name)
    #histData(entries, name)
    #plt.savefig(name + ".png")
    #plt.show()

process("data_sep.txt")
#process("data_native.txt")
#process("data_native2.txt")
#process("data_num.txt")
