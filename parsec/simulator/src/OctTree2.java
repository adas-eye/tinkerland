package edu.rit.pj.parsec;

import java.util.*;
import edu.rit.pj.Comm;
import edu.rit.pj.*;
import edu.rit.pj.parsec.Particle;

/*
 * OctTree - 3D quad tree, used here to optimize the calculation of
 * repulsive forces in n-body type simulations.
 *
 * At this time restructuring is done by rebuilding the tree
 */

public class OctTree {

    /* ORDER OF OCTANTS, counter clockwise starting at y=0, x axis
       0: NEE
       1: NNE
       2: NNW
       3: NWW
       4: SWW
       5: SSW
       6: SSE
       7: SEE

       0: X+, Y+, Z+
       1: X-, Y+, Z+
       2: X-, Y-, Z+
       3: X+, Y-, Z+
       4: X+, Y+, Z-
       5: X-, Y+, Z-
       6: X-, Y-, Z-
       7: X+, Y-, Z-
    */
    private OctTree children[]=new OctTree[8];

    /* List of Particles per quadrant. Correspond to octant order */
    private Particle data=null;

    //private static final int MAX_NODES = 5;   //maximum nodes per octant
    //private static final MAX_DEPTH = 50;  //the deepest we will go
    private int level=0;       //how deep are we?  0 for root

	//Position
	private double x=0.0;
	private double y=0.0;
	private double z=0.0;

	private int radius=Integer.MAX_VALUE;


    /*
     * Beyond the root
     */
    private OctTree( int l, List<Particle> lp ) {
        level = l;
        children = new OctTree[8];
        data = new ArrayList<<ArrayList<Particle>>();
        max_x = Integer.MAX_VALUE/l;
        max_y = Integer.MAX_VALUE/l;
        max_z = Integer.MAX_VALUE/l;
        min_x = -max_x;
		min_y = -max_y;
		min_z = -max_z;
        //redistribute nodes that were in this octant (parent)
        for( int i = 0; i < lp.size(); i++ ) {
            add( lp.get(i);
        }

    }

    /*
     * init
     */
    public OctTree() {
        //Do nothing
    }

    /*
     * Looks at each particle's position and adds to the appropriate
     * quadrant. If full, sub-divide again and pass this particle.
     */
    public void addData(Particle p) {
        //while level < 50
        //for this particle, attempt to add to list at appropriate octant
        //if not initialized, obv fill in a list
        //if octant !full, add particle
        //else  - repartition
           //if child does not exist, create and pass particle
           //else recurse with subtree
       while( level < 50 ) {
           int oct = findOctant(p);
           if( ArrayList<Particle> tmp1 = data.get(oct) == null ) {
                   tmp1 = new ArrayList<Particle>();
                   data.set(oct, tmp1);
           }
           List<Particle> lst = data.get(oct);
           int size = lst.size();
           if (size < MAX_NODES ) {
               lst.add(oct, p);
               data.set(oct, lst); //replace list, NOT add another!
           } else {
               children[oct] = new OctTree( (level + 1), children[oct] );
               children[oct].add(oct, p);
               data.set(oct, null);
               //think this works..
               children[oct].data.set(oct, lst);
           }
       }
    }

    // which to choose based on position?
    public int findOctant( Particle p ) {
        return 0;
    }

    // Recurse through whole tree from this level and position
    // return all particles in tree from here
    // If a level is invalid OR nothing is in this spot, return empty list
    // could pass in strings: NNE, SWW, etc.. and just check/convert to int
    public List<Particle> getValues( int level, int pos ) {
        //while not the correct level and the next octant is not null
        //once on level, return data.get(oct)

        //while traversing, check if children[pos] is not null. If it
        //isn't, don't look in this level's data List! Those nodes are redistributed

        // not impl.. let me know if this is correct search criteria!
        return data.get(oct);

    }

}

