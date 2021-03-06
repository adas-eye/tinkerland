//package sequential;

//import edu.rit.pj.Comm;

import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FileNotFoundException;
import java.io.IOException;

public class Parsec {
	private static Particle[] particles = new Particle[0];
	private static Particle[] newparticles = null;
	private static Particle[] tempRef = null;
	private static int[][] graph = new int[0][0];
	static long runtime=0;

	private Parsec() {}

	private static void readInData(String relationshipfilename, String posFileName) throws IOException, FileNotFoundException {
		String relbuf, posbuf;
		List<Particle> particles_l = new ArrayList<Particle>();
		List<int[]> graph_l = new ArrayList<int[]>();

		BufferedReader relationsreader = new BufferedReader(new FileReader(relationshipfilename));
		BufferedReader positionsreader = new BufferedReader(new FileReader(posFileName));
		while ((relbuf = relationsreader.readLine()) != null &&
				(posbuf = positionsreader.readLine()) != null) {
			if (relbuf.length()==0 || posbuf.length()==0) {
				break;
			}

			String[] relsplit = relbuf.split(",");
			String[] possplit = posbuf.split(",");

			Particle p=new Particle(relsplit[2], Integer.parseInt(relsplit[0]));
			for(int i=0; i<3; i++){
				p.pos[i]=Double.parseDouble(possplit[i]);
			}

			int[] adjacencies = new int[relsplit.length-3];
			for (int i = 3; i < relsplit.length; i++) {
				adjacencies[i-3] = Integer.parseInt(relsplit[i]);
			}
			particles_l.add(p);
			graph_l.add(adjacencies);
		}
		particles = particles_l.toArray(particles);
		graph = graph_l.toArray(graph);
	}

	private static void runSimulation(final double dt, BufferedWriter outbuf) throws Exception {
		final double threshold = 0.01;
		final double spring_k = 1.0;
		final double gravity_g = 0.005;
		final double friction_k = 0.05;
		int iterations = 0;
		double maxenergy = 0.0;
		do {
			maxenergy = 0.0;
			// Calculate the net force on particle p
			for (int i = 0; i < particles.length; i++) {
				double netforce[]={0.0, 0.0, 0.0};
				// Calculate attracting force
				// f = -kx for particles attached in the graph
				// This can be parallelized a bunch.

				for (int j = 0; j < particles.length; j++) {
					if (j == i) {
						for (int k = 0; k < graph[j].length; k++) {
							for(int d=0; d<3; d++){
									netforce[d]+=(particles[graph[j][k]].pos[d] - particles[i].pos[d])*spring_k;
								}
						}
					}
					else {
						for (int k = 0; k < graph[j].length; k++) {
							if (graph[j][k] == i) {
								for(int d=0; d<3; d++){
									netforce[d]+=(particles[j].pos[d] - particles[i].pos[d])*spring_k;
								}
							}
						}
					}
				}

				// Calculate repelling force
				// f = g*m1*m2/r^2
				for (int j = 0; j < particles.length; j++) {
					if (j != i) {
						double diff[]={0.0, 0.0, 0.0};
						double diffMag=0.0;
						for(int d=0; d<3; d++){
							diff[d]=particles[i].pos[d] - particles[j].pos[d];
							diffMag+=diff[d]*diff[d];
						}
						//diffMag=Math.sqrt(diffMag);

						double repel_coef=(gravity_g*particles[i].mass*particles[j].mass)/(diffMag*Math.sqrt(diffMag));

						for(int d=0; d<3; d++){
							netforce[d]+=(diff[d]*repel_coef);
						}
					}
				}
				// Calculate friction
				// f = mew_k * n = mew_k * m * g = friction_k * m
				if(particles[i].momentumSqrSum > 0.0){
					double diff[]={0.0, 0.0, 0.0};
					double momentumMag=Math.sqrt(particles[i].momentumSqrSum);
					for(int d=0; d<3; d++){
						netforce[d]-=(particles[i].momentum[d]/momentumMag)*friction_k*particles[i].mass;
					}
				}

				// Step the particle
				newparticles[i].copy(particles[i]);
				newparticles[i].stepParticle(netforce, dt);
				StringBuffer sbuf=new StringBuffer();
				for (int d=0; d<3; d++) {
					sbuf.append(newparticles[i].pos[d]);
					if (d<2) {
						sbuf.append(",");
					}
				}
				sbuf.append('\n');
				outbuf.write(sbuf.toString(), 0, sbuf.length());

				if(newparticles[i].energy > maxenergy){
					maxenergy=newparticles[i].energy;
				}
			}

			//Shuffle buffers
			tempRef=particles;
			particles = newparticles;
			newparticles=tempRef;
			// Calculate termination condition
			iterations++;
		} while (/*iterations < 50*/maxenergy > threshold);
		outbuf.write('\n');
	}

	public static void main(String[] args) throws Exception {
		//Comm.init (args);

		if (args.length < 2) {
			System.err.println("Usage: java Parsec [relationshipsfile] [positionsfile] <outputFile>");
			System.exit(-1);
		}
		try {
			System.out.println("Reading data... ");
			readInData(args[0], args[1]);
		}
		catch (FileNotFoundException e) {
			e.printStackTrace();
			System.exit(-1);
		}
		catch (IOException e) {
			e.printStackTrace();
			System.exit(-1);
		}

		newparticles=new Particle[particles.length];
		for(int i=0; i<particles.length; i++){
			newparticles[i]=new Particle(particles[i]);
		}

		BufferedWriter outbuffer=null;
		if(args.length==3){
			outbuffer=new BufferedWriter(new FileWriter(args[2]));
			System.out.println("Simulating...");
		}
		else{
			outbuffer=new BufferedWriter(new OutputStreamWriter(System.out));
		}

		runtime=System.currentTimeMillis();
		runSimulation(0.1, outbuffer);
		outbuffer.flush();
		runtime=System.currentTimeMillis()-runtime;

		outbuffer.close();
		System.out.println(runtime+"ms");
	}

}
