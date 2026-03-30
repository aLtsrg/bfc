[This program computes factorials. We won't assume that cells are bytes, or that they're not bytes, because implementations vary greatly on this point. Since factorials grow large, this means breaking the numbers up across multiple cells. (Even if correctness didn't require this, it would be very good for speed.) We want to output the numbers in decimal, so we'll store one decimal digit per cell.

We need to store n and n! (we'll call n! f in comments). We store the numbers with their digits interspersed, most significant digits to the right. Each time through the main loop, we increment n, output f, and multiply f by the new n, in place. (We do this one digit at a time, from right to left; each digit of f is output and then broken down in the process of multiplying it by each digit of n, with the partial products accumulated in the appropriate f cells. Updated parts of f will be called F in comments.)

Another wrinkle is that we want to represent all digits with nonzero cell values, as a way to distinguish between zeroes that are part of the numbers and zeroes that are empty array space. Generally we represent digits 0-9 using cell values 1-10, respectively, in both n and f; individual digits are decremented to return them to their "raw" form when needed for calculations.

The basic memory layout changes from:
0 0 0 0 f 0 0 f 0 0 f ... f 0 n 0 0 n 0 0 n ...
to
0 0 0 n f 0 n f 0 n f ...
and back again as n moves left during multiplication and then back to the right.

This program aims at a combination of speed and concision.]

>>>>++>+[
0 0 0 0 f 11' n 
0 0 0 0 2  1' 0 0
Initialization (normally this will be an 11 but the first time a 1 works fine)
This is the outermost loop done one time per factorial

    [
    ? 11' n 0 0 n?
    We need to increment n; we do this with two loops: an outer loop that sets
    all trailing 9s to 0s looking for a non9 and an inner loop for once we
    find one (perversely I have put almost the whole program into that
    inner loop) 
     
        >[>>]<[>+>]
        ? 11 n 0' 0 n?
        When we lengthen n (say when 999 is incremented to make 1000) we first
        add a leading 0 (cell value 1) just in time and then process it as
        usual; this code increments the n cell we're about to check only if
        its cell value was previously zero

        <<[>->>+<<<-]>+[
        0 0 (n minus 10)' 0 11 n?
        We subtract 11 from this digit of n and add 1; if it was cell value 10
        (digit 9) it's now 0 so we skip the inner loop and go on to the next
        digit; if the digit was not 9 we dive into the inner loop to output f
        and compute the next factorial
      
            [+>>[<<+>>-]>]+[-<<+<] add 11 add 1 to increment and to pad
            0 0 0 0 f 0 0 f 0 0 ``` 0' f 0 n 1 0 n 1 0 n 1 ``` n 1 0 0 0 0
            We want to pad f with extra leading zeroes (cell value 1) to the
            length of n so that as we multiply later we can just add value into
            these cells knowing f has already been lengthened to include them
            Either this will be exactly as many cells as needed to hold the new
            factorial or it'll be one cell extra
            So we go to the right end of n and then sweep left its whole length
            doing that (in passing we restore and increment that first digit of
            n that wasn't a 9); also we increment each n by an extra 1 and then
            decrement again when going left (as a kludgy way of saving a byte)
 
            >-[
            0 0 0 0 f 0 0 f 0 0 ``` 0 0 f' 0 n nf 0 n nf 0 n nf ``` n nf 0 0 0  
            We iterate this loop once per digit of the old f (to output it and
            multiply it by n)

                -[<+>>+<-]++++++[>++++++++<-]+>.[-]<<[
                0 0 0 0 f 0 0 ``` 0 f' F 0 n F 0 n F 0 n F ``` n nf 0 0 0  
                Return digit to raw form; make two copies (incidentally moving
                it out of the space its replacement will occupy and leaving
                that space as 0 (cell value 1))
                One copy gets 48 added to output the ASCII for that digit and
                is then erased; other copy will control this multiplication
                loop (each iteration adds all digits of n onto the right
                digits of new F)

                    >>>[[<<+>+>-]>>>]<<<<[[>+<-]<-<<]>-
                    Copy every digit of n onto F and into scratch space; then
                    move the copies from scratch space back to n; decrement F
                    to get the effect of adding the raw version of n although
                    we are copying the marked or "digit" version; decrement
                    old f digit we're multiplying (we do this loop that many
                    times)
                    
                ]>>>[
                Done with multiplication loop; now we need to do carries on F
                (cell values may be as great as 91 which is safe; but if we
                waited until the end to do carries rather than after every
                digit of f this program would overflow and would give wrong
                answers from 899! onward on a byte interpreter)

                This loop processes carry for one digit of F per iteration:
                it divides each digit by 10 putting the remainder back in
                that digit and dropping the quotient into the next digit
                right; but for speed it's crucial that we don't do this the
                whole length of F but only the length of n PLUS any F cells
                beyond that which get disrupted by carries

                We solve this by controlling the loop with n but padding n
                temporarily with leading zeroes (cell value 1) to mark any
                extra cells that have been touched

                Incidentally this is also where we move all n digits 3 cells
                left to set them up for the next left digit of f

                    <<-[<<+>>-]<+++++++++<[
                    Setup for division loop; we move this F "digit" (dividend)
                    out of the way (in raw form) to put the remainder in its
                    place; and we make a 9 to divide by 10; then enter the
                    division loop

                        >[->+>]>>>[<<[<+>-]>>>+>>[-<]<[>]>+<]<<<<<<-
                        This is a somewhat classic brainfuck division loop; it's
                        iterated (dividend) times and each time leaves the
                        quotient and remainder at the right values for the
                        number of times it's been iterated so far; usually it
                        increments remainder and decrements a counter; every
                        10th time it instead increments the quotient and moves
                        the value from the remainder to restore that counter
                        (and in this version also checks if the next n digit is
                        nonexistent (cell value 0) and if so pads it as a digit
                        0 (cell value 1) to make sure the carry loop is done at
                        least one more time)

                    ]>[-]>+>>[<<<+>>>-]>>>
                    Done with division; clear counter and unraw remainder; move
                    a digit of n left; go to next

                ]<<<+[-[+>>]<<<]>[<<<]>
                Done with carries; all F cell values are now 1 through 10; clear
                all leading 0s from n and return to next digit of f

            ]>>>[<[>>>]<<<[[>>>+<<<-]<<<]>>>>>>>-[<]>>>[<<]<<[>+>]<]<<
            Done outputting that factorial and computing the next one; now we
            need to move n back to the right past F; outer loop checks whether
            we're past F or not (if not then move all of n right three cells
            and check again); incidentally this is where we strip the possible
            leading 0 (cell value 1) from F: if we find an F cell of value 1
            followed by an F cell of value 0 we clear the 1; there can't be more
            than one (x digit number times y digit number gives a product either
            x plus y digits long or one shorter than that)

        ]++>>
        This is the end of the monster inner "if" loop within the initial
        increment of n; if we're still incrementing and looking for a non9 digit
        and skipped the inner loop then this will set the cell value of the last
        9 to 2* and will then bring the pointer to our 11 to continue looking
        for a non9 where we can increment n; if we have done that inner loop
        then that 11 is long gone and this loop drops us at a 0 to get out of
        the outer increment loop

        *(one as cell value for digit 0 and one more for kludge; it'll be
        decremented again when we sweep back left past it adding padding for F)

    ]<<++++++++.+
    Now we've finished incrementing n and also output the last factorial and
    computed the next one; now we output a linefeed and set up an 11 to use to
    look for non9 digits the next time we increment n

]
Done; this program doesn't terminate
