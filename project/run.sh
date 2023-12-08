# Create out_dir if it doesn't exist
mkdir -p out_dir

# Create in_dir/inputs if it doesn't exist
mkdir -p in_dir/inputs

# Copy files from inputs to in_dir/inputs
cp -r inputs/* in_dir/inputs

# compile the program
gcc -Wall -o prog piper.c

# run the program
./prog in_dir/inputs out_dir a
