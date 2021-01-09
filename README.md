# URC Million Neighborhoods Project Application - Code Sample

## TL;DR  

For interest only in technical implementation and fancy methods: [PressureSensor Details](#pressuresensor-details), [Analyzer Details](#analyzer-details)  
For interest in project workflow: [How does it run?](#how-does-it-run?)  
For interest only pretty graphs: [Grapher Details](#grapher-details)  

## A brief overview of the project

#### NOTE: for a more thorough overview, I recommend looking at the final section in this README. for a technical overview, I recommend reading through the poster pdf

My lab is interested in colloidal nanoparticles: what happens when many nanoparticles are spread out in a single layer. This is often called a "2D material", and the ones I studied are known to be unusually strong--imagine a single sheet of tissue paper being able to lift a bowling ball without tearing. These materials would make excellent candidates for technologies such as ultra-thin microelectronics for solar cells, or perhaps embedding chemical catalysts (e.g. the metals inside your car's catalytic converter) among the nanoparticles, then having the 2D material fold itself into a useful shape (e.g. high surface-area forms, which increase catalytic efficiency).

Unfortunately, we still don't know *how* these 2D materials work. The 2D film is made by depositing the nanoparticles onto the surface of water, and then using barriers to confine the monolayer to a rectangular shape. The nanoparticles are naturally attracted to one another, but we found that when we use these barriers to slowly compress the monolayer, the strength of the material increases exponentially. We measure this strength using Wilhelmy plates, which are small metal rectangles hung upright via string so that their bottom edge are in the monolayer. As the strength of the monolayer increases, it pulls the Wilhelmy plates down into the water; the tugging on the string is recorded and when integrated over monolayer area, is the strength. This strength eventually reaches a critical point, and the monolayer finally collapses--imagine a piece of paper wrinkling when you push two opposite edges together. The strength decreases to low values nearly instantaneously after the collapse, so knowing when collapse will happen will be vital in developing technologies down the road.

I worked on implementing these experiments as molecular dynamics simulations. The first goal was to demonstrate that the simulations replicated experimental values, so that the strength curves from my simulations and from the experiments match (and they do). The ongoing goal is to understand the mechanism of collapse in the monolayer using only the thermodynamic variables (pressure, free energy, entropy, temperature, etc.), and to use these relationships to control/direct collapse in useful ways. 

The simulations were run on the HPC Cluster Midway2 at the University of Chicago. The nanoparticles were discrete and used standard Monte Carlo MD algorithms (Verlet). The analysis, extensive data transformations, and data storage/management were performed entirely by me, as my PIs provided support only for theory, not for practice. In general, I used simulations of ~10,000 nanoparticles, which generated files on the order of 1-10GB per simulation in raw output. Analysis required dramatically more memory (100s of GB for small, 3-4k particle simulations), so I developed analysis techniques that required no memory beyond the raw output data. These files are code samples and are complete, but will not run without the engine nor raw data files, neither of which I am able to make public at this time.

## How does it run?

I designed this code to be as streamlined as possible, so that it is possible to run very different simulations and only having to change one file to do so. The 'simulation' is a *class* rather than a function, allowing for easy parallel computing as each simulation is an instance and does not depend on shared resources. This idea was extended to the analysis and graphical methods, and the simulation instance will automatically pass on the correct files and constants to the analysis, which ~prevents me from using the wrong set of values for two weeks~ reduces human error between runs.

The simulation is specified in the 'runner' file, on here as `runner_CAURS.py`. All of the necessary python libraries are imported, including those necessary for the classes instantiated later (i.e. each library has global access only so there aren't 6 versions of numpy open at the same time). `DASH` is the molecular dynamics (MD) engine and we specify the build + location. The constants and parameters (e.g. potential function, box size, compression rate, etc) are specified, and last are the command codes (they designate number of time steps, whether to compress or not, and the 'fixes' for DASH, which are C++ commands in a python wrapper). A simulation instance is created, and the the run() command runs the simulation. 

The simulation class (`simulator.py`) mostly functions to automate populating the DASH functions for the specified parameters, which makes it much easier to run these simulations--no need to rewrite the simulations or have a million copy-pasted versions with minor parameter changes. Since DASH requires CUDA programs that specify each potential function (included in `/DASH_RL_potentials`), I ended up making simulation subclasses for each potential as we needed it (`SimulatorRL.py`, to allow for different numbers of instance variables across simulations.

Once the simulation is done, an .xyz file is spit out, which specifies the xyz location of each particle at each timestep of the simulation. Its filename is chosen in the `runner` prior to the simulation run and automatically finds and loads the output file. 

We next specify the `Grapher` instance we would like to use (allows us to graph the same data multiple ways without re-running the analysis or saving the otherwise prohibitively large data set). 

The `PressureSensor` instance is then created, which defines a region in the simulation where we pretend there is a Wilhelmy plate (as with the `simulation` instance, we have subclasses, `PressureSensorRL.py` and `PressureSensor2Body.py`, which define different potential functions and their integrated forms). 

At last the `analyzer.py` instance is created, and the `PressureSensor` and `Grapher` passed to it. The .xyz file is opened and read, and the main `analyzer` function (`perform_analysis()`) is called. Details on all of these classes is provided below.

Finally, when done, the .xyz file is closed to preserve the data. The .xyz file and all graphs, charts, and data files created by the `Grapher`s are tidily stored in the same directory, and the full simulation/analysis/visualization is complete. The .xyz file can be put into a visualizer such as VMD to observe particle behavior, and screenshots of this are included in the `/images/` folder (the three screenshots are from VMD and show the results of compression). 


### `PressureSensor` Details  

The `PressureSensor` mimics the data collected by Wilhelmy plates, but it does not mimic the mechanism. Instead, I set out a permanent region (i.e. a region that does not disappear as a result of the compression), and found all of the particles inside that region using `PressureSensor.find_particles_in()`. I then manually calculate the surface pressure on each particle inside the region by `PressureSensor.calc_pressure()`, and similarly, I can find and store surface potential (necessary to find tensile strength, see poster for eqns) with `PressureSensor.calc_pot()`. 

It is necessary to calculate either the force or the potential between each pair of particles (Pressure = Force / Area, Potential = Integral(Force/Area)). Both are functions of only the distance between two particles. It is easiest to hard-code these functions instead of taking numerical integrals, and the hard-coding is kept in each `PressureSensor` subclass. Hence the choice to use `PressureSensorRL` for RL simulations, etc.

A `sensor` requires a frame as well as a border, which was chosen to ensure that the particles at the edge of the bounds do not lose the force effects of their nearest neighbors, but also so that these nearest neighbors do not have their own effective forces added to the Wilhelmy plate. The pressure for each timestep is calculated and stored as an instance variable in the `sensor`, and for convenience, the total area of the simulation at that time step is also stored as an instance variable. Note: **total area** is a proxy for time and is the independent variable/x-axis in most of the graphs, as the area decreases at each time step; it is not used in the surface pressure calculations--they use **`PressureSensor` area**.  


### `Analyzer` Details  

Analysis presented considerable problems to this pipeline. Each timestep requires significant computational analysis, as the process described above was done to ~500 particles for each of the 530,000 timesteps. Additionally, the .xyz file (really just a .txt file) was too large to hold in memory at the same time, even with the supercomputer. Parsing the file had to happen simultaneously with the computational analysis at each time step.

Because of this, I chose to build a [generator function](https://wiki.python.org/moin/Generators), implemented in `analyzer.parse_file()` and called in `analyzer.perform_analysis()`. `parse_file()` parses one timestep at a time, and once it has read all the data in the time step, `yield`s the timestep data to `perform_analysis`. The `analyzer` instance then finds the particles inside the `sensor` *frame* (the larger boundary), reducing the number of particle positions in memory from 10,000 to 500 or so.

Even with the greatly reduced particle set, finding the pairwise potentials between all pairs in this set is prohibitively expensive. Instead, we argue on physical grounds that the attractive forces between particles are directly the result of the 'velcro effect' and require direct contact, and so only a particle's nearest neighbors are relevant in the force calculation.

Nearest neighbors are then found through `analyzer.find_nearest_neighbors`. Using the python library `Voronoi` (i.e. [Qhull](http://www.qhull.org/html/index.htm), which uses the DeLaunay triangularization algorithms implemented in C), I created a 3D Voronoi tesselation of the particles, which is computationally very cheap. The tesselation will return its ridge points, which are the lines drawn from particle to particle that form the Voronoi cells; the ridge points themselves are pairs of particles, e.g. \[4, 8] means a ridge forms between particles 4 and 8. Using this set of ridge points, I was able to find the nearest neighbors.

The final technical note is in the use of a dictionary to store the nearest neighbors. When iterating over the ridge points, each unique particle becomes a dictionary key (specifically, the tuple of its (x,y,z) position is the key), and a list of its nearest neighbors is the value. Since python dictionaries are hash tables, this dropped computational cost to zero relative to the I/O cost of reading the .xyz file.

When finding the total force or potential in the `PressureSensor`, I pass it this dictionary. In only iterating over the key/value pairs and evaluating the hard-coded functions, `PressureSensor.calculate_pressure()` also has negligible computational cost, and as soon as the pressure and area are calculated and saved in the `sensor` instance, the loop pass ends.  

The generator function `parse_file()` is called again to pick up where it left off, and the timestep data are deleted from memory. The next timestep is parsed, ... This loop repeats until the entire file has been analyzed.

In summary, by using a generator function, `Qhull` code, and python dictionaries, the massive overhead for the analysis of 530,000 timesteps is eliminated. Only the data for a single timestep is in memory at a time, and the 10,000 particles are reduced to ~500 almost immediately. The 500 are then Voronoi tesselated, placed in a hash table, and after simple arithmetic calculations, reduced to only two numbers. The two numbers are saved, then the entire timestep is wiped from memory. The analysis is computationally sparse enough to perform on my laptop and watch Youtube at the same time, and takes about 20 minutes to finish. 

#### `Grapher` Details  

Not much to say on technical implementation, just a bit of detail on the two images in `/images`. `isotherm.png` is an example of the graphed isotherms using a bit of smoothing; see `ljcaursplot.py`, but it's just fancy matplotlib. `height_dist.png` is an image I made while we were investigating the relationship between particle height distribution and the monolayer collapse. Each vertical slice of the graph is a histogram of the z-position of the particles. The histogram bin density is then converted into a matplotlib colorbar object (globalized a posteriori so that the colors played nice). After this has been done for all timesteps, the colorbar objects are aggregated and turned into a heatmap, which is the graph. This graph in particular led to significant insight in at the transition point, as it demonstrates that particles do not "spread out" from regions of high density to fill in the bilayer, but instead expel the furtherest particles and maintain the monolayer as best they can. Code for this graph is in `graphing_z_stuff.py`. 


## Some Background Information and Technical Details

Two-dimensional materials, often called single-layer materials, can be formed using nanoparticles. These materials may be tailored to suit a wide range of needs, with applications that span photovoltaics, microelectronics, and surface chemistry/catalysis (e.g. water purification). My lab was interested in 2D materials with unusually high tensile strengths (resistance to breaking under mechanical stresses like shear force), specifically thiol-ligated gold nanoparticle monolayers. These nanoparticles have a solid gold core, made of a few hundred gold atoms, which were then covered in hydrocarbon 'hairs' attached via sulfur atoms ("thiol-ligated"). The tensile strength of these layers is attributed to a velcro-like effect of the hairs with neighboring particles.  

Our goal for this project was to develop an empirical theory of the properties of these 2D films, so that it would be possible to precisely manipulate the films (potential direct applications included dispersing a chemical catalyst throughout the film, then transforming it into useful shapes like cylinders or high-surface-area forms, e.g.). One group on this project did wet lab work, where the nanoparticles are deposited on the surface of water, then compressed in different ways to force the films to self-assemble. Key to this was the use of Wilhelmy plates, which are very thin metal rectangles suspended by fishing wire so that the bottom of the plates are submerged. By orienting one perpendicular and the other parallel to the direction of compression, we may observe the change in surface pressure as the 'velcro effect' draws the nanoparticles closer together. The surface pressures are then integrated over the surface area of the film to find two useful measures of tensile strength: the Strain Modulus (resistance to stretching) and the Shear Modulus (resistance to twisting). Eventually these forces grow to the point of collapse, and the monolayer begins 'folding' into a bilayer.  

My project aimed to replicate these experimental results using simulations. A Pritzker Institute for Molecular Engineering graduate student built a molecular dynamics simulation engine that we adapted for use with our supercomputing cluster ([link to github page for the engine](https://github.com/dreid1991/md_engine)). By doing many first-principles MD simulations of two, three, and seven nanoparticles drawing closer together (i.e. simulating the nanoparticles by monitoring each carbon atom on the 'hairs' and measuring their attractive energies), we were able to develop empirical equations for the potential function--in other words, we had an equation that if we knew how far apart two nanoparticles were, we could calculate the strength of the velcro effect between them. The engine and the potential functions were then used for Monte Carlo methods to model how the monolayer changes under compression.  

The strain and stress moduli curves from the simulations were eventually found to match the experimental ones (using 7 nanoparticles to find the potential function yielded results on the same order as the results from experiment, whereas using only 2 or 3 only matched the general shape of the curves but overestimated their strength). From this, we were confident that the simulations could provide useful mechanistic insight to the collapse of the monolayer at the moments of high stress (and it's much cheaper to run simulations than to buy gold nanoparticles for each tiny change you want to make). Additionally, the simulations verified that the equilibrium assumptions of the experimental conditions held, which were important in many of its applications; the most important was that the collapse patterns that we observed, which formed diagonal striations across the layer, were a result of only interparticle interactions and totally independent of the compressions we were doing (i.e. the compression of the monolayer was slow enough that the system was at equilibrium at all times).   

In its current form, the project is focused on evaluating the effect of each of the key variables on monolayer collapse: 
* height distributions (do random fluctuations in particle height determine where collapse occurs?)
* packing and lattice effects (transition state theory, or in essence, do misalignments in packing density determine collapse location?)
* local pressure fluctuations (what does the surface pressure look like at points of interest, not just wherever a Wilhelmy plate is?)
* local free energy fluctuations (how much energy from attractions between nanoparticles is released when the bilayer collapses? is there an energy threshhold that determines when collapse happens? are the striations we observe a consequence of minimizing the energy of the bilayer?)
