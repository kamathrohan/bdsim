BDSIM="bdsim"
REBDSIM="rebdsim"
REBDSIMCOMBINE="rebdsimCombine"
REBDSIMOPTICS="rebdsimOptics"

# data samples
$BDSIM --file=originalmodels/sc.gmad --batch --ngenerate=10 --outfile=sample1 --seed=123
$BDSIM --file=originalmodels/sc.gmad --batch --ngenerate=10 --outfile=sample2 --seed=321

# same as sample1 but with minimal data
$BDSIM --file=originalmodels/minimal-data.gmad --batch --ngenerate=10 --outfile=minimal-data --seed=123

# trajectory data for testing
$BDSIM --file=originalmodels/trajectories.gmad --batch --ngenerate=2 --outfile=trajectory-sample --seed=123

# analysis
$REBDSIM analysisConfig.txt sample1.root ana1.root
$REBDSIM analysisConfig.txt sample2.root ana2.root
$REBDSIMCOMBINE combined-ana.root ana1.root ana2.root

# optics analysis
$BDSIM --file=originalmodels/fodo.gmad --batch --ngenerate=100 --outfile=fodo --seed=456
$REBDSIMOPTICS fodo.root optics.root
$BDSIM --file=originalmodels/fodo-no-primaries.gmad --batch --ngenerate=100 --outfile=fodo-no-primaries --seed=654

# job with an event generator -> to test recreation with such a setup
# seed and ngenerate are set internally for that model
$BDSIM --file=../beam/eventgeneratorfile/eventgeneratorfile-hepmc2.gmad --batch --outfile=../beam/eventgeneratorfile/egf-hepmc2-output

# more complex data for spectra
$BDSIM --file=originalmodels/shower.gmad --batch --outfile=shower
$BDSIM --file=originalmodels/shower_cyl_sph.gmad --batch --outfile=shower_cyl_sph

