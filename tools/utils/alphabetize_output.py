"""By default, Amanzi/ATS through XDMF generates output visualization
files that don't alphabetize naturally, breaking Matlab's HDF5 reader.
This tries to alphabetize them by prepending all timestep strings with
0's to pad as needed.
"""

import h5py

def alphabetize(datin, datout):
    for var in datin.keys():
        grp = datout.create_group(var)
        keys = datin[var].keys()
        keys.sort(lambda a,b: int.__cmp__(int(a), int(b)))
        nints = len(keys[-1])

        fmt = "%0"+str(nints)+"d"

        for k in keys:
            knew = fmt%int(k)
            grp.create_dataset(knew, data=datin[var][k][:])

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("FILENAME_IN", type=str)
    parser.add_argument("FILENAME_OUT", type=str)

    args = parser.parse_args()

    with h5py.File(args.FILENAME_IN, 'r') as fin, h5py.File(args.FILENAME_OUT, 'w') as fout:
        alphabetize(fin, fout)
    
        
    
