(set-logic BV)

(define-fun mymax ((x (_ BitVec 32)) (y(_ BitVec 32))) (_ BitVec 32) 
( bvor ( ite ( bvule y x ) #x00000001 #x00000000 ) ( ite ( bvule y ( ite ( bvule y x ) #x00000001 #x00000000 ) ) #x00000001 #x00000000 ) ))

(declare-var x (BitVec 32) )
(declare-var y (BitVec 32) )

; property
(assert (<= (mymax x y) x))
(assert (<= (mymax x y) y))

(assert (or (= (mymax x y) x) (= (mymax x y) y)))

(check-sat)


