requests['Display all actors'] = "SELECT * FROM actors;"
requests['Display all genres'] = "SELECT * FROM genres;"
requests['Display the name and year of the movies'] = "SELECT mov_title, mov_year FROM movies;"
requests['Display reviewer name from the reviews table'] = "SELECT rev_name FROM reviews;"

requests["Find the year when the movie American Beauty released"] = "SELECT mov_year FROM movies WHERE mov_title = 'American Beauty';"
requests["Find the movie which was released in the year 1999"] = "SELECT mov_title FROM movies WHERE mov_year = 1999;"
requests["Find the movie which was released before 1998"] = "SELECT mov_title FROM movies WHERE mov_year < 1998;"
requests["Find the name of all reviewers who have rated 7 or more stars to their rating order by reviews rev_name (it might have duplicated names :-))"] = "SELECT rev_name FROM reviews, movies_ratings_reviews WHERE reviews.id = movies_ratings_reviews.rev_id AND movies_ratings_reviews.rev_stars >= 7 ORDER BY rev_name;"
requests["Find the titles of the movies with ID 905, 907, 917"] = "SELECT mov_title FROM movies WHERE id IN (905, 907, 917);"
requests["Find the list of those movies with year and ID which include the words Boogie Nights"] = "SELECT id, mov_title, mov_year FROM movies WHERE mov_title LIKE '%Boogie Nights%';"
requests["Find the ID number for the actor whose first name is 'Woody' and the last name is 'Allen'"] = "SELECT id FROM actors WHERE act_fname = 'Woody' AND act_lname = 'Allen';"

requests["Find the actors with all information who played a role in the movies 'Annie Hall'"] = "SELECT actors.* FROM actors, movies_actors, movies WHERE actors.id = movies_actors.act_id AND movies.id = movies_actors.mov_id AND movies.mov_title = 'Annie Hall';"
requests["Find the first and last names of all the actors who were cast in the movies 'Annie Hall', and the roles they played in that production"] = "SELECT actors.act_fname, actors.act_lname, movies_actors.role FROM actors, movies_actors, movies WHERE actors.id = movies_actors.act_id AND movies.id = movies_actors.mov_id AND movies.mov_title = 'Annie Hall';"

requests["Find the name of movie and director who directed a movies that casted a role as Sean Maguire"] = "SELECT DISTINCT directors.dir_fname, directors.dir_lname, movies.mov_title FROM movies, movies_actors, directors_movies, directors WHERE movies.id = movies_actors.mov_id AND movies.id = directors_movies.mov_id AND directors.id = directors_movies.dir_id AND movies_actors.role = 'Sean Maguire';"
requests["Find all the actors who have not acted in any movie between 1990 and 2000 (select only actor first name, last name, movie title and release year)"] =
  "SELECT actors.act_fname, actors.act_lname, movies.mov_title, CAST(strftime('%Y', movies.mov_dt_rel) AS INT) \
   FROM actors, movies_actors, movies \
   WHERE actors.id = movies_actors.act_id \
     AND movies.id = movies_actors.mov_id \
     AND CAST(strftime('%Y', movies.mov_dt_rel) AS INT) NOT BETWEEN 1990 AND 2000 \
     AND actors.id NOT IN ( \
           SELECT movies_actors.act_id \
           FROM movies_actors, movies \
           WHERE movies.id = movies_actors.mov_id \
             AND CAST(strftime('%Y', movies.mov_dt_rel) AS INT) BETWEEN 1990 AND 2000 \
     );"
