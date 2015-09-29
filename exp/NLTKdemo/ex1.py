#/usr/bin/env python
import nltk
sentence = """At eight o'clock on Thursday morning
Arthur didn't feel very good."""
tokens = nltk.word_tokenize(sentence)
tagged = nltk.pos_tag(tokens)
#nltk.download() # downloads all nltk modules and corpora
entities = nltk.chunk.ne_chunk(tagged)
type(entities) # class 'nltk.tree.Tree'
print entities
entities.draw()
